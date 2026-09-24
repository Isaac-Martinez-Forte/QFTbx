/**
 * @file
 * @brief The ray-crossing count of the Nyquist criterion on the Nichols chart.
 *
 * The plant's poles are placed once: the number in the right half-plane is
 * the P of the criterion, the ones on the imaginary axis are where the loop
 * passes through infinity, and a plant that cannot place them gets no
 * verdict rather than one that assumes none. The loop at unit gain is
 * sampled over the base grid one factor at a time, and wherever the phase
 * turns faster than the unwrapping tolerance a sample is inserted at the
 * geometric mean of the frequencies, within a budget. A curve that starts on
 * a ray counts half a crossing towards where it departs. Profiles are cached
 * up to a size of the order of tens of megabytes.
 */

#include "src/core/loopshaping/common/nominal_stability_checker.h"

#include "src/core/math/polynomial.h"
#include "src/core/math/constants.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "src/core/common/exception.h"
#include "src/core/system/parameter.h"

namespace qftbx {

namespace {

const double kRayMagnitude = 1.0;

const std::size_t kMaxCachedProfiles = 200000;

double phaseDegrees(double re, double im)
{
    return std::atan2(im, re) * 180.0 / qftbx::math::kPi;
}

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

double rayBelow(double phaseDegrees)
{
    return std::floor((phaseDegrees + 180.0) / 360.0) * 360.0 - 180.0;
}

int side(double im)
{
    return im > 0.0 ? 1 : (im < 0.0 ? -1 : 0);
}

std::uint64_t bitsOf(double value)
{
    if (value == 0.0) {
        value = 0.0;
    }
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

}

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
        throw InvalidInput(QFTBX_TR("Core", "The stability check needs at least one design frequency."));
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

    const std::optional<std::vector<std::complex<double>>> poles = m_plant->nominalPoles();
    if (!poles.has_value()) {
        throw InvalidInput(QFTBX_TR("Core", "The stability criterion cannot place the poles of the nominal plant: its denominator is not a polynomial in s."));
    }
    m_rhpPoles = math::rightHalfPlaneCount(*poles);
    m_axisPoles = math::imaginaryAxisFrequencies(*poles);

    m_plantAtZero = m_plant->evaluate(0.0);

    std::vector<double> numerator, denominator;
    for (Parameter & parameter : m_plant->numerator()) numerator.push_back(parameter.nominal());
    for (Parameter & parameter : m_plant->denominator()) denominator.push_back(parameter.nominal());
    const std::optional<LtiSystem::Polynomials> polynomials =
            m_plant->polynomialsAt(numerator, denominator, m_plant->gain().nominal());
    if (polynomials.has_value() && m_plant->delay().nominal() == 0.0 && !m_plant->delay().isUncertain()) {
        const auto lowest = [](const std::vector<double> & p, int & zerosAtOrigin) {
            zerosAtOrigin = 0;
            for (auto it = p.rbegin(); it != p.rend(); ++it) {
                if (*it != 0.0) return *it;
                ++zerosAtOrigin;
            }
            return 0.0;
        };
        int numeratorZeros = 0, denominatorZeros = 0;
        const double n = lowest(polynomials->numerator, numeratorZeros);
        const double d = lowest(polynomials->denominator, denominatorZeros);
        if (n != 0.0 && d != 0.0) {
            m_asymptoteKnown = true;
            m_plantOrderAtZero = numeratorZeros - denominatorZeros;
            m_plantCoefficientAtZero = n / d;
        }
    }
}

std::size_t NominalStabilityChecker::axisPolesBetween(double lo, double hi) const
{
    std::size_t count = 0;
    for (const double pole : m_axisPoles) {
        if (pole > lo && pole < hi) {
            ++count;
        }
    }
    return count;
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
        double step = std::abs(phaseDegrees(br, bi) - phaseDegrees(ar, ai));
        if (step > 180.0) {
            step = 360.0 - step;
        }
        return step > m_tolerances.maxPhaseStepDegrees;
    }

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

    int budget = m_tolerances.refinementBudget;
    for (std::size_t i = 0; i + 1 < m_w.size() && budget > 0;) {
        if (phaseStepExceeded(i) && m_w[i + 1] - m_w[i] > 1e-12 * m_w[i]
                && (m_axisPoles.empty() || axisPolesBetween(m_w[i], m_w[i + 1]) == 0)) {
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

    double atZeroFactor = sign;
    int order = m_plantOrderAtZero;
    for (const double zero : shape.zeros) {
        if (zero == 0.0) ++order; else atZeroFactor *= zero;
    }
    for (const double pole : shape.poles) {
        if (pole == 0.0) --order; else atZeroFactor /= pole;
    }

    bool startKnown = false;
    double startPhase = 0.0;
    double startMagnitude = 0.0;
    if (m_asymptoteKnown) {
        const double coefficient = atZeroFactor * m_plantCoefficientAtZero;
        if (order <= 0) {
            startKnown = true;
            startPhase = (coefficient < 0.0 ? 180.0 : 0.0) + 90.0 * order;
            startPhase -= 360.0 * std::floor((startPhase + 180.0) / 360.0);
            startMagnitude = order < 0 ? std::numeric_limits<double>::infinity() : std::abs(coefficient);
        }
    } else {
        const std::complex<double> atZero = atZeroFactor * m_plantAtZero;
        if (std::isfinite(atZero.real()) && std::isfinite(atZero.imag())
                && (atZero.real() != 0.0 || atZero.imag() != 0.0)) {
            startKnown = true;
            startPhase = phaseDegrees(atZero.real(), atZero.imag());
            startMagnitude = std::abs(atZero);
        }
    }
    if (!startKnown) {
        startPhase = phaseDegrees(m_re[0], m_im[0]);
        startMagnitude = std::hypot(m_re[0], m_im[0]);
    }
    const double startRay = rayBelow(startPhase + 1e-6);
    if (std::abs(startPhase - startRay) < 1e-3) {
        profile.startsOnRay = true;
        profile.startMagnitudeAtUnitGain = startMagnitude;

        double accumulated = 0.0;
        double previous = startPhase;
        for (std::size_t next = startKnown ? 0 : 1; next + 1 < count; ++next) {
            const double phase = phaseDegrees(m_re[next], m_im[next]);
            accumulated += wrappedDelta(previous, phase);
            previous = phase;
            if (std::abs(accumulated) >= 1e-9) {
                break;
            }
        }
        if (std::isinf(startMagnitude)) {
            profile.startDirection = accumulated > 0.0 ? 0.0 : -1.0;
        } else {
            profile.startDirection = accumulated > 0.0 ? 0.5 : -0.5;
        }
    }

    const bool anyAxisPole = !m_axisPoles.empty();

    for (std::size_t i = 0; i + 1 < count; ++i) {
        const std::size_t poles = anyAxisPole ? axisPolesBetween(m_w[i], m_w[i + 1]) : 0;

        if (poles == 0 && side(m_im[i]) * side(m_im[i + 1]) >= 0) {
            continue;
        }
        const double a = phaseDegrees(m_re[i], m_im[i]);
        double delta = wrappedDelta(a, phaseDegrees(m_re[i + 1], m_im[i + 1]));

        if (poles > 0) {
            const double expected = -180.0 * static_cast<double>(poles);
            delta -= 360.0 * std::round((delta - expected) / 360.0);
        }

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

bool NominalStabilityChecker::isStable(const Profile & profile, double gainMagnitude) const
{
    if (!profile.decided) {
        return false;
    }

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

    return std::abs(crossings - 0.5 * m_rhpPoles) < 0.25;
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

bool NominalStabilityChecker::isBoxUnstable(LtiSystem * box, NaturalIntervalExtension & extension)
{
    constexpr std::size_t kStride = 8;

    const PointController corner = cornerOf(box, true);
    const Profile & profile = profileOf(corner);
    ++m_statistics.verdicts;
    if (!profile.decided || isStable(profile, std::abs(corner.gain))) {
        return false;
    }

    const std::size_t n = m_frequencies.size();
    const auto reachesCriticalPoint = [&](std::size_t i) {
        const NicholsBox enclosure = extension.nicholsBox(box, m_frequencies[i],
                                                          std::complex<double>(m_plantRe[i], m_plantIm[i]));
        return enclosure.magnitudeDb.lower() <= 0.0 && enclosure.magnitudeDb.upper() >= 0.0 &&
               enclosure.phaseDegrees.lower() <= -180.0 && enclosure.phaseDegrees.upper() >= -180.0;
    };
    for (std::size_t i = 0; i < n; i += 64) {
        if (reachesCriticalPoint(i)) {
            return false;
        }
    }
    for (std::size_t i = 0; i < n; i += kStride) {
        if (i % 64 != 0 && reachesCriticalPoint(i)) {
            return false;
        }
    }
    return true;
}

}
