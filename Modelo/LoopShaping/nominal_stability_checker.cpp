#include "nominal_stability_checker.h"

#include <algorithm>
#include <cmath>

#include "src/core/system/parameter.h"

namespace qftbx {

namespace {

const int kBaseGridPoints = 3000;
const qreal kDecadesBeyond = 3.0;
//Maximum phase step between consecutive samples before refining: keeps
//the unwrapping unambiguous with a wide margin.
const qreal kMaxPhaseStepDegrees = 30.0;
const int kRefinementBudget = 200000;
//A loop is counted as crossing a ray only above this magnitude (0 dB).
const qreal kRayMagnitude = 1.0;

qreal phaseDegrees(const std::complex<qreal> & value)
{
    return std::arg(value) * 180.0 / M_PI;
}

} // namespace

NominalStabilityChecker::NominalStabilityChecker(LtiSystem * nominalPlant,
                                                 QVector<qreal> * omega)
    : m_plant(nominalPlant)
{
    qreal minOmega = omega->first();
    qreal maxOmega = omega->first();

    foreach (qreal o, *omega) {
        minOmega = std::min(minOmega, o);
        maxOmega = std::max(maxOmega, o);
    }

    const qreal logFrom = std::log10(minOmega) - kDecadesBeyond;
    const qreal logTo = std::log10(maxOmega) + kDecadesBeyond;

    m_frequencies.reserve(kBaseGridPoints);
    m_plantValues.reserve(kBaseGridPoints);

    for (int i = 0; i < kBaseGridPoints; ++i) {
        const qreal w = std::pow(10.0, logFrom + (logTo - logFrom) * i / (kBaseGridPoints - 1));
        m_frequencies.push_back(w);
        m_plantValues.push_back(m_plant->evaluate(w));
    }
}

std::complex<qreal> NominalStabilityChecker::plantAt(qreal w)
{
    return m_plant->evaluate(w);
}

//Zero-pole-gain semantics, matching NaturalIntervalExtension::nicholsBox:
//C(jw) = k prod(jw + z_i) / prod(jw + p_j) over the nominal values.
std::complex<qreal> NominalStabilityChecker::controllerAt(LtiSystem * controller, qreal w)
{
    const std::complex<qreal> jw(0.0, w);

    std::complex<qreal> value(controller->gain()->nominal(), 0.0);

    foreach (Parameter * zero, *controller->numerator()) {
        value *= jw + std::complex<qreal>(zero->nominal(), 0.0);
    }

    foreach (Parameter * pole, *controller->denominator()) {
        value /= jw + std::complex<qreal>(pole->nominal(), 0.0);
    }

    return value;
}

bool NominalStabilityChecker::isNominallyStable(LtiSystem * pointController)
{
    //The working curve: frequency, |L0| and unwrapped phase, refined where
    //the phase turns faster than the unwrapping tolerance.
    struct Sample {
        qreal w;
        qreal magnitude;
        qreal phase; //raw in (-180, 180], unwrapped afterwards
    };

    std::vector<Sample> curve;
    curve.reserve(m_frequencies.size());

    for (std::size_t i = 0; i < m_frequencies.size(); ++i) {
        const std::complex<qreal> loop =
                controllerAt(pointController, m_frequencies[i]) * m_plantValues[i];
        curve.push_back({m_frequencies[i], std::abs(loop), phaseDegrees(loop)});
    }

    //Adaptive refinement: subdivide any interval whose raw phase step
    //exceeds the tolerance (resonant plants turn 180 degrees in a very
    //narrow band). The raw step is a valid proxy: the true step can only
    //alias once it exceeds 180 degrees, far above the tolerance.
    int budget = kRefinementBudget;

    for (std::size_t i = 0; i + 1 < curve.size() && budget > 0;) {
        qreal step = std::abs(curve[i + 1].phase - curve[i].phase);
        if (step > 180.0) {
            step = 360.0 - step;
        }

        if (step > kMaxPhaseStepDegrees && curve[i + 1].w - curve[i].w > 1e-12 * curve[i].w) {
            const qreal w = std::sqrt(curve[i].w * curve[i + 1].w);
            const std::complex<qreal> loop = controllerAt(pointController, w) * plantAt(w);
            curve.insert(curve.begin() + i + 1, {w, std::abs(loop), phaseDegrees(loop)});
            --budget;
        } else {
            ++i;
        }
    }

    if (budget <= 0) {
        //The phase could not be resolved: refuse to certify.
        return false;
    }

    //The loop must be proper: the criterion closes the contour where the
    //magnitude has fallen below the ray magnitude.
    if (curve.back().magnitude >= kRayMagnitude) {
        return false;
    }

    //Unwrap the phase into a continuous curve.
    std::vector<qreal> unwrapped(curve.size());
    unwrapped[0] = curve[0].phase;

    for (std::size_t i = 1; i < curve.size(); ++i) {
        qreal delta = curve[i].phase - curve[i - 1].phase;

        if (delta > 180.0) {
            delta -= 360.0;
        } else if (delta < -180.0) {
            delta += 360.0;
        }

        unwrapped[i] = unwrapped[i - 1] + delta;
    }

    //Net signed crossings of the rays phase = -180 (mod 360) with
    //|L0| > 0 dB. The sign only needs to be consistent: stability for a
    //nominally stable open loop is net == 0.
    qreal crossings = 0.0;

    const auto rayBelow = [](qreal phase) {
        //Largest ray level (-180 + 360 m) at or below 'phase'.
        return std::floor((phase + 180.0) / 360.0) * 360.0 - 180.0;
    };

    //A curve starting ON a ray with gain above 0 dB (two integrators put
    //the low-frequency phase exactly at -180) counts half a crossing
    //towards the side it departs to.
    const qreal startRay = rayBelow(unwrapped[0] + 1e-6);
    if (std::abs(unwrapped[0] - startRay) < 1e-3 && curve[0].magnitude > kRayMagnitude) {
        std::size_t next = 1;
        while (next + 1 < curve.size() && std::abs(unwrapped[next] - unwrapped[0]) < 1e-9) {
            ++next;
        }
        crossings += (unwrapped[next] > unwrapped[0]) ? 0.5 : -0.5;
    }

    for (std::size_t i = 0; i + 1 < curve.size(); ++i) {
        const qreal a = unwrapped[i];
        const qreal b = unwrapped[i + 1];

        if (a == b) {
            continue;
        }

        //Every ray level strictly between a and b is a crossing.
        const qreal low = std::min(a, b);
        const qreal high = std::max(a, b);
        const qreal sign = (b > a) ? 1.0 : -1.0;

        for (qreal level = rayBelow(high); level > low; level -= 360.0) {
            if (level >= high) {
                continue;
            }

            //Magnitude at the crossing, log-interpolated in frequency.
            const qreal t = (level - a) / (b - a);
            const qreal magnitude = curve[i].magnitude *
                    std::pow(curve[i + 1].magnitude / curve[i].magnitude, t);

            if (magnitude > kRayMagnitude) {
                crossings += sign;
            }
        }
    }

    return std::abs(crossings) < 0.25;
}

} // namespace qftbx
