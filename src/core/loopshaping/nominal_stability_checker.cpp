#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/constants.h"

#include <vector>
#include <algorithm>
#include <cmath>

#include "src/core/common/exception.h"
#include "src/core/system/parameter.h"

namespace qftbx {

namespace {

//The grid resolution used to live here; it comes from the settings now, so
//it can be traded for time without a rebuild. What stays is the one number
//that is NOT a setting:
//A loop is counted as crossing a ray only above this magnitude (0 dB).
const double kRayMagnitude = 1.0;

double phaseDegrees(const std::complex<double> & value)
{
    return std::arg(value) * 180.0 / qftbx::math::kPi;
}

} // namespace

NominalStabilityChecker::NominalStabilityChecker(LtiSystem * nominalPlant,
                                                 std::vector<double> * omega,
                                                 Settings::Stability tolerances)
    : m_plant(nominalPlant), m_tolerances(tolerances)
{
    if (omega == nullptr || omega->empty()) {
        throw InvalidInput("The stability check needs at least one design frequency.");
    }

    double minOmega = omega->front();
    double maxOmega = omega->front();

    for (double o : *omega) {
        minOmega = std::min(minOmega, o);
        maxOmega = std::max(maxOmega, o);
    }

    const double logFrom = std::log10(minOmega) - m_tolerances.decadesBeyond;
    const double logTo = std::log10(maxOmega) + m_tolerances.decadesBeyond;

    m_frequencies.reserve(m_tolerances.baseGridPoints);
    m_plantValues.reserve(m_tolerances.baseGridPoints);

    for (int i = 0; i < m_tolerances.baseGridPoints; ++i) {
        const double w = std::pow(10.0, logFrom + (logTo - logFrom) * i / (m_tolerances.baseGridPoints - 1));
        m_frequencies.push_back(w);
        m_plantValues.push_back(m_plant->evaluate(w));
    }
}

std::complex<double> NominalStabilityChecker::plantAt(double w)
{
    return m_plant->evaluate(w);
}

//Zero-pole-gain semantics, matching NaturalIntervalExtension::nicholsBox:
//C(jw) = k prod(jw + z_i) / prod(jw + p_j) over the point's values.
std::complex<double> NominalStabilityChecker::controllerAt(const PointController & controller, double w)
{
    const std::complex<double> jw(0.0, w);

    std::complex<double> value(controller.gain, 0.0);

    for (const double zero : controller.zeros) {
        value *= jw + std::complex<double>(zero, 0.0);
    }

    for (const double pole : controller.poles) {
        value /= jw + std::complex<double>(pole, 0.0);
    }

    return value;
}

bool NominalStabilityChecker::isNominallyStable(LtiSystem * controller)
{
    //Every parameter at its nominal value, which is what a point system
    //holds; the values are read once here instead of once per sample.
    PointController point;
    point.gain = controller->gain().nominal();

    point.zeros.reserve(controller->numerator().size());
    for (Parameter & zero : controller->numerator()) {
        point.zeros.push_back(zero.nominal());
    }

    point.poles.reserve(controller->denominator().size());
    for (Parameter & pole : controller->denominator()) {
        point.poles.push_back(pole.nominal());
    }

    return isNominallyStable(point);
}

bool NominalStabilityChecker::isNominallyStable(const PointController & pointController)
{
    //The working curve: frequency, the loop value and its raw phase,
    //refined where the phase turns faster than the unwrapping tolerance.
    //The magnitude is read from the loop value where the criterion looks
    //at it - the last sample, a start on a ray, the two ends of a crossing
    //- rather than computed for every sample: the check runs on hundreds of
    //thousands of candidates per search, and most samples never need it.
    struct Sample {
        double w;
        std::complex<double> loop;
        double phase; //raw in (-180, 180], unwrapped afterwards

        double magnitude() const { return std::abs(loop); }
    };

    std::vector<Sample> curve;
    curve.reserve(m_frequencies.size());

    for (std::size_t i = 0; i < m_frequencies.size(); ++i) {
        const std::complex<double> loop =
                controllerAt(pointController, m_frequencies[i]) * m_plantValues[i];
        curve.push_back({m_frequencies[i], loop, phaseDegrees(loop)});
    }

    //Adaptive refinement: subdivide any interval whose raw phase step
    //exceeds the tolerance (resonant plants turn 180 degrees in a very
    //narrow band). The raw step is a valid proxy: the true step can only
    //alias once it exceeds 180 degrees, far above the tolerance.
    int budget = m_tolerances.refinementBudget;

    for (std::size_t i = 0; i + 1 < curve.size() && budget > 0;) {
        double step = std::abs(curve[i + 1].phase - curve[i].phase);
        if (step > 180.0) {
            step = 360.0 - step;
        }

        if (step > m_tolerances.maxPhaseStepDegrees && curve[i + 1].w - curve[i].w > 1e-12 * curve[i].w) {
            const double w = std::sqrt(curve[i].w * curve[i + 1].w);
            const std::complex<double> loop = controllerAt(pointController, w) * plantAt(w);
            curve.insert(curve.begin() + static_cast<std::ptrdiff_t>(i) + 1, {w, loop, phaseDegrees(loop)});
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
    if (curve.back().magnitude() >= kRayMagnitude) {
        return false;
    }

    //Unwrap the phase into a continuous curve.
    std::vector<double> unwrapped(curve.size());
    unwrapped[0] = curve[0].phase;

    for (std::size_t i = 1; i < curve.size(); ++i) {
        double delta = curve[i].phase - curve[i - 1].phase;

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
    double crossings = 0.0;

    const auto rayBelow = [](double phase) {
        //Largest ray level (-180 + 360 m) at or below 'phase'.
        return std::floor((phase + 180.0) / 360.0) * 360.0 - 180.0;
    };

    //A curve starting ON a ray with gain above 0 dB (two integrators put
    //the low-frequency phase exactly at -180) counts half a crossing
    //towards the side it departs to.
    const double startRay = rayBelow(unwrapped[0] + 1e-6);
    if (std::abs(unwrapped[0] - startRay) < 1e-3 && curve[0].magnitude() > kRayMagnitude) {
        std::size_t next = 1;
        while (next + 1 < curve.size() && std::abs(unwrapped[next] - unwrapped[0]) < 1e-9) {
            ++next;
        }
        crossings += (unwrapped[next] > unwrapped[0]) ? 0.5 : -0.5;
    }

    for (std::size_t i = 0; i + 1 < curve.size(); ++i) {
        const double a = unwrapped[i];
        const double b = unwrapped[i + 1];

        if (a == b) {
            continue;
        }

        //Every ray level strictly between a and b is a crossing.
        const double low = std::min(a, b);
        const double high = std::max(a, b);
        const double sign = (b > a) ? 1.0 : -1.0;

        for (double level = rayBelow(high); level > low; level -= 360.0) {
            if (level >= high) {
                continue;
            }

            //Magnitude at the crossing, log-interpolated in frequency.
            const double t = (level - a) / (b - a);
            const double from = curve[i].magnitude();
            const double to = curve[i + 1].magnitude();
            const double magnitude = from * std::pow(to / from, t);

            if (magnitude > kRayMagnitude) {
                crossings += sign;
            }
        }
    }

    return std::abs(crossings) < 0.25;
}

} // namespace qftbx
