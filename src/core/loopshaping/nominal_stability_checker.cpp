#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/constants.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "src/core/common/exception.h"
#include "src/core/system/parameter.h"

namespace qftbx {

namespace {

//The rays are at |L| > 0 dB.
const double kRayMagnitude = 1.0;

//Profiles kept before the cache is emptied. A profile is a handful of
//doubles; this is of the order of tens of megabytes.
const std::size_t kMaxCachedProfiles = 200000;

double phaseDegrees(double re, double im)
{
    return std::atan2(im, re) * 180.0 / qftbx::math::kPi;
}

//The change of phase from a to b, in (-180, 180]: how the criterion
//unwraps consecutive samples.
double wrappedDelta(double fromDegrees, double toDegrees)
{
    double delta = toDegrees - fromDegrees;
    if (delta > 180.0) {
        delta -= 360.0;
    } else if (delta < -180.0) {
        delta += 360.0;
    }
    return delta;
}

//The ray at or below a phase: -180 + 360 k.
double rayBelow(double phaseDegrees)
{
    return std::floor((phaseDegrees + 180.0) / 360.0) * 360.0 - 180.0;
}

//Which side of the real axis a sample is on: 0 exactly on it.
int side(double im)
{
    return im > 0.0 ? 1 : (im < 0.0 ? -1 : 0);
}

std::uint64_t bitsOf(double value)
{
    //Both zeros hash alike, as they compare equal.
    if (value == 0.0) {
        value = 0.0;
    }
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

} // namespace

std::size_t NominalStabilityChecker::KeyHash::operator()(const std::vector<double> & key) const
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (const double value : key) {
        hash ^= bitsOf(value);
        hash *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(hash);
}

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
    m_plantRe.reserve(m_tolerances.baseGridPoints);
    m_plantIm.reserve(m_tolerances.baseGridPoints);
    for (int i = 0; i < m_tolerances.baseGridPoints; ++i) {
        const double w = std::pow(10.0, logFrom + (logTo - logFrom) * i / (m_tolerances.baseGridPoints - 1));
        const std::complex<double> value = m_plant->evaluate(w);
        m_frequencies.push_back(w);
        m_plantRe.push_back(value.real());
        m_plantIm.push_back(value.imag());
    }

    m_cosMaxPhaseStep = std::cos(m_tolerances.maxPhaseStepDegrees * qftbx::math::kPi / 180.0);
}

std::complex<double> NominalStabilityChecker::plantAt(double w)
{
    return m_plant->evaluate(w);
}

std::complex<double> NominalStabilityChecker::loopAt(const PointController & shape, double sign, double w)
{
    double re = sign;
    double im = 0.0;
    for (const double zero : shape.zeros) {
        const double r = re * zero - im * w;
        const double m = re * w + im * zero;
        re = r;
        im = m;
    }
    for (const double pole : shape.poles) {
        const double inverse = 1.0 / (pole * pole + w * w);
        const double r = (re * pole + im * w) * inverse;
        const double m = (im * pole - re * w) * inverse;
        re = r;
        im = m;
    }
    const std::complex<double> plant = plantAt(w);
    return std::complex<double>(re * plant.real() - im * plant.imag(),
                                re * plant.imag() + im * plant.real());
}

bool NominalStabilityChecker::phaseStepExceeded(std::size_t i) const
{
    const double ar = m_re[i], ai = m_im[i], br = m_re[i + 1], bi = m_im[i + 1];
    const double squaredNorms = (ar * ar + ai * ai) * (br * br + bi * bi);

    if (squaredNorms == 0.0) {
        //A sample exactly at the origin has no direction; the criterion
        //reads its phase as zero, as atan2 does.
        double step = std::abs(phaseDegrees(br, bi) - phaseDegrees(ar, ai));
        if (step > 180.0) {
            step = 360.0 - step;
        }
        return step > m_tolerances.maxPhaseStepDegrees;
    }

    //The angle between the two samples exceeds the tolerance when the
    //cosine of the angle, dot / (|a| |b|), is below the cosine of the
    //tolerance; squared to avoid the roots, with the signs handled apart.
    const double dot = ar * br + ai * bi;
    const double c = m_cosMaxPhaseStep;
    if (c >= 0.0) {
        return dot < 0.0 || dot * dot < c * c * squaredNorms;
    }
    return dot < 0.0 && dot * dot > c * c * squaredNorms;
}

NominalStabilityChecker::Profile NominalStabilityChecker::computeProfile(const PointController & shape)
{
    ++m_statistics.profilesComputed;
    Profile profile;

    const double sign = shape.gain < 0.0 ? -1.0 : 1.0;
    const std::size_t n = m_frequencies.size();

    //The loop at unit gain over the base grid, one factor at a time over
    //the whole grid: plain loops over arrays the compiler vectorises.
    m_w.assign(m_frequencies.begin(), m_frequencies.end());
    m_re.assign(n, sign);
    m_im.assign(n, 0.0);

    for (const double zero : shape.zeros) {
        for (std::size_t i = 0; i < n; ++i) {
            const double w = m_w[i];
            const double r = m_re[i] * zero - m_im[i] * w;
            const double m = m_re[i] * w + m_im[i] * zero;
            m_re[i] = r;
            m_im[i] = m;
        }
    }
    for (const double pole : shape.poles) {
        for (std::size_t i = 0; i < n; ++i) {
            const double w = m_w[i];
            const double inverse = 1.0 / (pole * pole + w * w);
            const double r = (m_re[i] * pole + m_im[i] * w) * inverse;
            const double m = (m_im[i] * pole - m_re[i] * w) * inverse;
            m_re[i] = r;
            m_im[i] = m;
        }
    }
    for (std::size_t i = 0; i < n; ++i) {
        const double r = m_re[i] * m_plantRe[i] - m_im[i] * m_plantIm[i];
        const double m = m_re[i] * m_plantIm[i] + m_im[i] * m_plantRe[i];
        m_re[i] = r;
        m_im[i] = m;
    }

    //Refinement: wherever the phase turns faster than the unwrapping
    //tolerance between two samples, a sample at the geometric mean of the
    //frequencies is inserted between them, within a budget.
    int budget = m_tolerances.refinementBudget;
    for (std::size_t i = 0; i + 1 < m_w.size() && budget > 0;) {
        if (phaseStepExceeded(i) && m_w[i + 1] - m_w[i] > 1e-12 * m_w[i]) {
            const double w = std::sqrt(m_w[i] * m_w[i + 1]);
            const std::complex<double> loop = loopAt(shape, sign, w);
            const auto at = static_cast<std::ptrdiff_t>(i) + 1;
            m_w.insert(m_w.begin() + at, w);
            m_re.insert(m_re.begin() + at, loop.real());
            m_im.insert(m_im.begin() + at, loop.imag());
            --budget;
        } else {
            ++i;
        }
    }

    if (budget <= 0) {
        return profile;
    }
    profile.decided = true;

    const std::size_t count = m_w.size();
    profile.lastMagnitudeAtUnitGain = std::hypot(m_re[count - 1], m_im[count - 1]);

    //A curve that starts on a ray: half a crossing towards where it
    //departs, when its magnitude is above the ray's.
    const double startPhase = phaseDegrees(m_re[0], m_im[0]);
    const double startRay = rayBelow(startPhase + 1e-6);
    if (std::abs(startPhase - startRay) < 1e-3) {
        profile.startsOnRay = true;
        profile.startMagnitudeAtUnitGain = std::hypot(m_re[0], m_im[0]);

        double accumulated = 0.0;
        double previous = startPhase;
        for (std::size_t next = 1; next + 1 < count; ++next) {
            const double phase = phaseDegrees(m_re[next], m_im[next]);
            accumulated += wrappedDelta(previous, phase);
            previous = phase;
            if (std::abs(accumulated) >= 1e-9) {
                break;
            }
        }
        profile.startDirection = accumulated > 0.0 ? 0.5 : -0.5;
    }

    //The crossings of the rays. Between two consecutive samples the
    //unwrapped phase moves by less than 180 degrees, so the arc can only
    //reach a ray when the samples lie on opposite sides of the real axis;
    //there the phase is interpolated linearly and the magnitude
    //geometrically, and the crossing counts with the direction of the
    //turn. Samples exactly on the axis are not a crossing, as the
    //criterion reads a ray strictly between two phases.
    for (std::size_t i = 0; i + 1 < count; ++i) {
        if (side(m_im[i]) * side(m_im[i + 1]) >= 0) {
            continue;
        }
        const double a = phaseDegrees(m_re[i], m_im[i]);
        const double delta = wrappedDelta(a, phaseDegrees(m_re[i + 1], m_im[i + 1]));
        if (delta == 0.0) {
            continue;
        }
        const double b = a + delta;
        const double low = std::min(a, b);
        const double high = std::max(a, b);
        const double level = rayBelow(high);
        if (!(level > low && level < high)) {
            continue;
        }
        const double t = (level - a) / delta;
        const double from = std::hypot(m_re[i], m_im[i]);
        const double to = std::hypot(m_re[i + 1], m_im[i + 1]);
        profile.crossings.push_back({from * std::pow(to / from, t), delta > 0.0 ? 1.0 : -1.0});
    }

    return profile;
}

const NominalStabilityChecker::Profile & NominalStabilityChecker::profileOf(const PointController & shape)
{
    m_key.clear();
    m_key.reserve(shape.zeros.size() + shape.poles.size() + 2);
    m_key.push_back(shape.gain < 0.0 ? -1.0 : 1.0);
    m_key.push_back(static_cast<double>(shape.zeros.size()));
    m_key.insert(m_key.end(), shape.zeros.begin(), shape.zeros.end());
    m_key.insert(m_key.end(), shape.poles.begin(), shape.poles.end());

    const auto found = m_profiles.find(m_key);
    if (found != m_profiles.end()) {
        return found->second;
    }

    if (m_profiles.size() >= kMaxCachedProfiles) {
        m_profiles.clear();
    }
    return m_profiles.emplace(m_key, computeProfile(shape)).first->second;
}

bool NominalStabilityChecker::isStable(const Profile & profile, double gainMagnitude)
{
    if (!profile.decided) {
        return false;
    }

    //Not proper: the loop does not fall below the rays at the top of the
    //grid.
    if (gainMagnitude * profile.lastMagnitudeAtUnitGain >= kRayMagnitude) {
        return false;
    }

    double crossings = 0.0;
    if (profile.startsOnRay && gainMagnitude * profile.startMagnitudeAtUnitGain > kRayMagnitude) {
        crossings += profile.startDirection;
    }
    for (const Crossing & crossing : profile.crossings) {
        if (gainMagnitude * crossing.magnitudeAtUnitGain > kRayMagnitude) {
            crossings += crossing.sign;
        }
    }

    return std::abs(crossings) < 0.25;
}

bool NominalStabilityChecker::isNominallyStable(LtiSystem * controller)
{
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

bool NominalStabilityChecker::isNominallyStable(const PointController & point)
{
    ++m_statistics.verdicts;
    return isStable(profileOf(point), std::abs(point.gain));
}

} // namespace qftbx
