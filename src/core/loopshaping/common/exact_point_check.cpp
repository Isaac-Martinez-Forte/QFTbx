/**
 * @file
 * @brief One candidate of the search against the specifications themselves,
 * and the exact gain set along the ray of its phase.
 */

#include "src/core/loopshaping/common/exact_point_check.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>

#include "src/core/common/exception.h"
#include "src/core/math/constants.h"
#include "src/core/math/convex_hull.h"
#include "src/core/math/line_envelope.h"
#include "src/core/math/quadratic_set.h"

namespace qftbx {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();
constexpr double kLn10 = 2.302585092994045684;
constexpr std::size_t kMaxRounds = 64;
constexpr double kLadder[] = {0.0, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10, 1e-9, 1e-8, 1e-7};

double toDb(double g)
{
    if (!(g > 0.0)) {
        return -kInfinity;
    }
    if (!(g < kInfinity)) {
        return kInfinity;
    }
    return 20.0 * std::log10(g);
}

double trackingFactor(double boundDb)
{
    return std::expm1(boundDb * kLn10 / 10.0);
}

struct Disc {
    double s;
    double t;
};

std::optional<Disc> discOf(SpecificationType type, double W, double modulus, double nominalModulus, double plantModulus)
{
    switch (type) {
    case SpecificationType::Stability:
    case SpecificationType::SensorNoise:
        return Disc{1.0 / W, 0.0};
    case SpecificationType::OutputDisturbance:
        return Disc{0.0, modulus / W};
    case SpecificationType::InputDisturbance:
        return Disc{0.0, nominalModulus / W};
    case SpecificationType::ControlEffort:
        if (!(plantModulus > 0.0)) {
            return std::nullopt;
        }
        return Disc{1.0 / (W * plantModulus), 0.0};
    case SpecificationType::TrackingLower:
    case SpecificationType::TrackingUpper:
        break;
    }
    return std::nullopt;
}

struct Arc {
    bool whole = false;
    double fromCosine = 1.0, fromSine = 0.0;
    double toCosine = 1.0, toSine = 0.0;
    double middleCosine = 1.0, middleSine = 0.0;
    double halfWidthCosine = 1.0;

    Arc(double from, double to)
    {
        if (to - from >= 2.0 * math::kPi) {
            whole = true;
            return;
        }
        fromCosine = std::cos(from);
        fromSine = std::sin(from);
        toCosine = std::cos(to);
        toSine = std::sin(to);
        middleCosine = std::cos(0.5 * (from + to));
        middleSine = std::sin(0.5 * (from + to));
        halfWidthCosine = std::cos(0.5 * (to - from));
    }

    double largestCosine(double cosine, double sine) const
    {
        if (whole || cosine * middleCosine + sine * middleSine >= halfWidthCosine) {
            return 1.0;
        }
        return std::max(cosine * fromCosine + sine * fromSine, cosine * toCosine + sine * toSine);
    }
};

double largestOver(double a, double b, double c, double g1, double g2)
{
    double largest = std::max(a * g1 * g1 + b * g1 + c, a * g2 * g2 + b * g2 + c);
    if (a < 0.0) {
        const double vertex = -b / (2.0 * a);
        if (vertex > g1 && vertex < g2) {
            largest = std::max(largest, a * vertex * vertex + b * vertex + c);
        }
    }
    return largest;
}

}

ExactPointCheck::ExactPointCheck(LtiSystem & plant, LtiSystem * controller, const std::vector<double> & omega,
                                 const CloudSet & templates, const SpecificationSet & specifications)
{
    if (controller == nullptr || templates.size() < omega.size()) {
        return;
    }

    m_controller = controller->clone();
    m_reference.emplace(plant, omega, templates, specifications);

    m_referenceOf.assign(omega.size(), std::numeric_limits<std::size_t>::max());
    for (std::size_t f = 0; f < m_reference->frequencies().size(); ++f) {
        const FrequencyReference & at = m_reference->frequencies()[f];
        m_referenceOf[at.index] = f;
        std::vector<std::size_t> seed = math::convexHullVertices(at.nominalOverValueSet);
        std::sort(seed.begin(), seed.end());
        m_hull.push_back(seed);

        std::vector<Quotient> quotients;
        quotients.reserve(at.nominalOverValueSet.size());
        for (std::size_t n = 0; n < at.nominalOverValueSet.size(); ++n) {
            const std::complex<double> qn = at.nominalOverValueSet[n];
            const double modulus = std::abs(qn);
            const double angle = std::arg(qn);
            quotients.push_back({modulus, std::cos(angle), std::sin(angle), std::norm(qn),
                                  std::abs((*at.valueSet)[n])});
        }
        m_quotients.push_back(std::move(quotients));
        m_pairs.emplace_back();
        m_sectors.emplace_back();
        m_statistics.largestWorkingSet = std::max(m_statistics.largestWorkingSet, seed.size());
        m_working.push_back(std::move(seed));
    }
}

void ExactPointCheck::requireUsable() const
{
    if (!usable()) {
        throw InvalidInput(QFTBX_TR("Core", "The exact point check has no value set for every design frequency."));
    }
}

std::complex<double> ExactPointCheck::loopAt(const FrequencyReference & at, const PointController & point) const
{
    return m_controller->valueAt(at.omega, point.zeros, point.poles, point.gain, m_controller->delay().nominal())
           * at.nominalPlant;
}

bool ExactPointCheck::admits(const PointController & point)
{
    requireUsable();
    return admitsFrom(point, m_firstToAsk);
}

bool ExactPointCheck::admitsFrom(const PointController & point, std::size_t & firstToAsk)
{
    ++m_statistics.verdicts;

    const std::vector<FrequencyReference> & frequencies = m_reference->frequencies();
    const std::size_t count = frequencies.size();

    for (std::size_t step = 0; step < count; ++step) {
        const std::size_t i = (firstToAsk + step) % count;
        ++m_statistics.kernelPasses;
        if (!(frequencies[i].worstExcessAt(loopAt(frequencies[i], point)) <= -kToleranceDb)) {
            firstToAsk = i;
            return false;
        }
    }

    return true;
}

SpecificationCheck ExactPointCheck::checkOf(const PointController & point) const
{
    requireUsable();

    SpecificationCheck check;
    for (const FrequencyReference & at : m_reference->frequencies()) {
        at.recordExcesses(loopAt(at, point), check);
    }
    return check;
}

void ExactPointCheck::admissibleMagnitudes(std::size_t frequency, std::complex<double> direction,
                                           const std::vector<std::size_t> & plants, RangeUnion & allowed)
{
    const FrequencyReference & at = m_reference->frequencies()[frequency];
    const std::vector<Quotient> & quotients = m_quotients[frequency];
    allowed.assign(0.0, kInfinity);

    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const auto cosine = [&](std::size_t n) { return std::real(std::conj(q[n]) * direction); };

    for (const FrequencyReference::Bound & bound : at.bounds) {
        if (allowed.isEmpty()) {
            break;
        }

        const double boundDb = bound.boundDb - kToleranceDb;

        if (bound.type == SpecificationType::TrackingLower) {
            trackingMagnitudes(frequency, direction, plants, boundDb, m_scratch.tracking);
            allowed.intersectWith(m_scratch.tracking);
            continue;
        }

        const double W = dbToLinear(boundDb);
        const double nominalModulus = std::abs(at.nominalPlant);
        const bool output = bound.type == SpecificationType::OutputDisturbance;
        const bool effort = bound.type == SpecificationType::ControlEffort;
        for (const std::size_t n : plants) {
            if (allowed.isEmpty()) {
                break;
            }
            const double c = cosine(n);
            const double q2 = quotients[n].norm;
            const std::optional<Disc> disc = discOf(bound.type, W, output ? std::sqrt(q2) : 0.0, nominalModulus,
                                                    effort ? quotients[n].plantModulus : 0.0);
            if (!disc.has_value()) {
                allowed.clear();
                return;
            }
            const double s = disc->s, t = disc->t;
            math::whereNonNegative(1.0 - s * s, 2.0 * (c - s * t), q2 - t * t, m_scratch.quadratic);
            allowed.intersectWith(m_scratch.quadratic);
        }
    }
}

void ExactPointCheck::trackingMagnitudes(std::size_t frequency, std::complex<double> direction,
                                         const std::vector<std::size_t> & plants, double boundDb, RangeUnion & set)
{
    const FrequencyReference & at = m_reference->frequencies()[frequency];
    const std::vector<Quotient> & quotients = m_quotients[frequency];
    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const auto cosine = [&](std::size_t n) { return std::real(std::conj(q[n]) * direction); };

    const double dm1 = trackingFactor(boundDb);
    const double d2 = dm1 + 1.0;

    std::vector<math::Line> & lines = m_scratch.lines;
    lines.clear();
    for (const std::size_t n : plants) {
        lines.push_back({2.0 * cosine(n), quotients[n].norm, n});
    }
    std::vector<math::EnvelopePiece> & nearest = m_scratch.nearest;
    std::vector<math::EnvelopePiece> & farthest = m_scratch.farthest;
    math::envelopes(lines, m_scratch.envelope, nearest, farthest);

    std::vector<double> & lower = m_scratch.trackingLower;
    std::vector<double> & upper = m_scratch.trackingUpper;
    lower.clear();
    upper.clear();
    RangeUnion & piece = m_scratch.trackingPiece;
    std::size_t i = 0, j = 0;
    double from = 0.0;
    while (i < nearest.size() && j < farthest.size()) {
        const double nextNearest = i + 1 < nearest.size() ? nearest[i + 1].from : kInfinity;
        const double nextFarthest = j + 1 < farthest.size() ? farthest[j + 1].from : kInfinity;
        const double to = std::min(nextNearest, nextFarthest);
        const std::size_t n = nearest[i].index;
        const std::size_t m = farthest[j].index;

        math::whereNonNegative(dm1, 2.0 * (d2 * cosine(n) - cosine(m)), d2 * quotients[n].norm - quotients[m].norm,
                               piece);
        piece.intersectWith(from, to);
        for (const Range & part : piece.components()) {
            lower.push_back(part.min);
            upper.push_back(part.max);
        }

        if (to == kInfinity) {
            break;
        }
        from = to;
        if (nextNearest == to) ++i;
        if (nextFarthest == to) ++j;
    }
    set.assign(lower.data(), upper.data(), lower.size());
}

RangeUnion ExactPointCheck::admissibleGainsDbOver(const std::vector<double> & zeros, const std::vector<double> & poles,
                                                  Range gainRange,
                                                  const std::vector<std::vector<std::size_t>> * workingSets)
{
    RangeUnion gains = RangeUnion::of(toDb(gainRange.min), toDb(gainRange.max));

    const std::vector<FrequencyReference> & frequencies = m_reference->frequencies();
    const double delay = m_controller->delay().nominal();

    for (std::size_t f = 0; f < frequencies.size() && !gains.isEmpty(); ++f) {
        const FrequencyReference & at = frequencies[f];
        if (at.bounds.empty()) {
            continue;
        }

        const std::complex<double> unit = m_controller->valueAt(at.omega, zeros, poles, 1.0, delay) * at.nominalPlant;
        const double mu = std::abs(unit);
        if (!(mu > 0.0) || !(mu < kInfinity)) {
            continue;
        }
        const double muDb = toDb(mu);

        std::vector<std::size_t> & everyPlant = m_scratch.everyPlant;
        if (workingSets == nullptr) {
            everyPlant.resize(at.nominalOverValueSet.size());
            std::iota(everyPlant.begin(), everyPlant.end(), std::size_t(0));
        }
        RangeUnion & magnitudes = m_scratch.magnitudes;
        admissibleMagnitudes(f, unit / mu, workingSets == nullptr ? everyPlant : (*workingSets)[f], magnitudes);

        std::vector<double> & lower = m_scratch.gainLower;
        std::vector<double> & upper = m_scratch.gainUpper;
        lower.clear();
        upper.clear();
        for (const Range & part : magnitudes.components()) {
            lower.push_back(toDb(part.min) - muDb);
            upper.push_back(toDb(part.max) - muDb);
        }
        m_scratch.gainParts.assign(lower.data(), upper.data(), lower.size());
        gains.intersectWith(m_scratch.gainParts);
    }

    return gains;
}

RangeUnion ExactPointCheck::admissibleGainsDb(const std::vector<double> & zeros, const std::vector<double> & poles,
                                              Range gainRange)
{
    requireUsable();
    return admissibleGainsDbOver(zeros, poles, gainRange, nullptr);
}

bool ExactPointCheck::growWorkingSet(std::size_t frequency, const PointController & point)
{
    const FrequencyReference & at = m_reference->frequencies()[frequency];
    const std::complex<double> loop = loopAt(at, point);
    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const std::vector<Quotient> & quotients = m_quotients[frequency];
    const bool outputInForce = at.mask.outputDisturbance;
    const bool effortInForce = at.mask.controlEffort;
    const double loopModulus = std::abs(loop);

    std::size_t nearest = 0, farthest = 0, worstOutput = 0, worstEffort = 0;
    double dMin = kInfinity, dMax = -kInfinity, outputMax = -kInfinity, effortMax = -kInfinity;
    for (std::size_t n = 0; n < q.size(); ++n) {
        const double d = std::abs(q[n] + loop);
        if (d < dMin) { dMin = d; nearest = n; }
        if (d > dMax) { dMax = d; farthest = n; }
        if (outputInForce) {
            const double output = quotients[n].modulus / d;
            if (output > outputMax) { outputMax = output; worstOutput = n; }
        }
        if (effortInForce) {
            const double effort = loopModulus / (quotients[n].plantModulus * d);
            if (effort > effortMax) { effortMax = effort; worstEffort = n; }
        }
    }

    std::vector<std::size_t> & set = m_working[frequency];
    bool grew = false;
    const auto add = [&](std::size_t n) {
        const auto position = std::lower_bound(set.begin(), set.end(), n);
        if (position == set.end() || *position != n) {
            set.insert(position, n);
            grew = true;
        }
    };
    add(nearest);
    add(farthest);
    if (outputInForce) add(worstOutput);
    if (effortInForce) add(worstEffort);

    m_statistics.largestWorkingSet = std::max(m_statistics.largestWorkingSet, set.size());
    return grew;
}

ExactPointCheck::GainSearch ExactPointCheck::lowestAdmissibleGain(const std::vector<double> & zeros,
                                                                  const std::vector<double> & poles, Range gainRange)
{
    requireUsable();

    GainSearch result;
    ++m_statistics.gainSearches;

    bool grew = true;
    for (std::size_t round = 0; round < kMaxRounds && grew; ++round) {
        const RangeUnion set = admissibleGainsDbOver(zeros, poles, gainRange, &m_working);

        grew = false;
        for (const Range & component : set.components()) {
            const double lo = dbToLinear(component.min);
            const double hi = dbToLinear(component.max);
            const double middle = 0.5 * (lo + hi);
            double previous = -1.0;

            for (const double eta : kLadder) {
                const double gain = std::clamp(std::min(lo * (1.0 + eta), middle), gainRange.min, gainRange.max);
                if (gain == previous) {
                    continue;
                }
                previous = gain;

                const PointController candidate{gain, zeros, poles};
                ++result.confirmations;
                if (admitsFrom(candidate, m_ladderFirstToAsk)) {
                    result.gain = gain;
                    break;
                }
                if (growWorkingSet(m_ladderFirstToAsk, candidate)) {
                    grew = true;
                    break;
                }
                ++result.ladderSteps;
            }
            if (grew || result.gain.has_value()) {
                break;
            }
        }

        if (grew) {
            ++result.rounds;
        } else if (!result.gain.has_value() && !set.isEmpty()) {
            ++m_statistics.laddersExhausted;
        }
    }
    if (grew) {
        ++m_statistics.roundLimitsReached;
    }

    m_statistics.exchangeRounds += result.rounds;
    m_statistics.ladderSteps += result.ladderSteps;
    return result;
}

ExactPointCheck::SectorVerdict ExactPointCheck::sectorVerdict(std::size_t omegaIndex, Range phaseDegrees,
                                                              Range magnitudeDb)
{
    requireUsable();

    SectorVerdict verdict;
    if (omegaIndex >= m_referenceOf.size() || m_referenceOf[omegaIndex] == std::numeric_limits<std::size_t>::max()) {
        return verdict;
    }
    ++m_statistics.sectorVerdicts;

    const std::size_t f = m_referenceOf[omegaIndex];
    RememberedSector & remembered = m_sectors[f];
    if (remembered.valid && remembered.workingSize == m_working[f].size()
            && std::memcmp(&remembered.phase, &phaseDegrees, sizeof(Range)) == 0
            && std::memcmp(&remembered.magnitude, &magnitudeDb, sizeof(Range)) == 0) {
        return remembered.verdict;
    }
    const FrequencyReference & at = m_reference->frequencies()[f];
    const std::vector<Quotient> & quotients = m_quotients[f];
    const Arc arc(phaseDegrees.min * math::kPi / 180.0, phaseDegrees.max * math::kPi / 180.0);
    const double g1 = dbToLinear(magnitudeDb.min);
    const double g2 = dbToLinear(magnitudeDb.max);

    std::vector<double> & lower = m_forbiddenLower;
    std::vector<double> & upper = m_forbiddenUpper;
    lower.clear();
    upper.clear();

    for (std::size_t k = 0; k < at.bounds.size(); ++k) {
        const FrequencyReference::Bound & bound = at.bounds[k];
        if (bound.type == SpecificationType::TrackingLower) {
            const double dm1 = trackingFactor(bound.boundDb);
            for (const TrackingPair & pair : trackingPairs(f, k)) {
                const double rho = pair.modulus * arc.largestCosine(pair.cosine, pair.sine);
                const double a = dm1, b = 2.0 * rho, c = pair.constant;
                if (a > 0.0 && b >= 0.0 && c >= 0.0) {
                    continue;
                }
                if (largestOver(a, b, c, g1, g2) < 0.0) {
                    verdict.provablyInfeasible = true;
                }
                math::appendWhereNegative(a, b, c, lower, upper);
            }
            continue;
        }

        const double W = dbToLinear(bound.boundDb);
        const double nominalModulus = std::abs(at.nominalPlant);
        for (const Quotient & quotient : quotients) {
            const std::optional<Disc> disc = discOf(bound.type, W, quotient.modulus, nominalModulus, quotient.plantModulus);
            if (!disc.has_value()) {
                continue;
            }
            const double s = disc->s, t = disc->t;
            const double cmax = quotient.modulus * arc.largestCosine(quotient.cosine, quotient.sine);
            const double a = 1.0 - s * s, b = 2.0 * (cmax - s * t), c = quotient.norm - t * t;
            if (a > 0.0 && b >= 0.0 && c >= 0.0) {
                continue;
            }
            if (largestOver(a, b, c, g1, g2) < 0.0) {
                verdict.provablyInfeasible = true;
            }
            math::appendWhereNegative(a, b, c, lower, upper);
        }
    }

    if (!lower.empty()) {
        double covered = 0.0;
        bool grew = true;
        while (grew) {
            grew = false;
            for (std::size_t k = 0; k < lower.size(); ++k) {
                if (lower[k] <= covered && upper[k] > covered) {
                    covered = upper[k];
                    grew = true;
                }
            }
        }
        if (covered > 0.0) {
            verdict.forbiddenBelowDb = toDb(covered);
        }
        double reached = kInfinity;
        grew = true;
        while (grew) {
            grew = false;
            for (std::size_t k = 0; k < lower.size(); ++k) {
                if (upper[k] >= reached && lower[k] < reached) {
                    reached = lower[k];
                    grew = true;
                }
            }
        }
        if (reached < kInfinity) {
            verdict.forbiddenAboveDb = toDb(reached);
        }
    }

    if (verdict.forbiddenBelowDb >= magnitudeDb.max || verdict.forbiddenAboveDb <= magnitudeDb.min) {
        verdict.provablyInfeasible = true;
    }

    remembered = {true, m_working[f].size(), phaseDegrees, magnitudeDb, verdict};
    return verdict;
}

const std::vector<ExactPointCheck::TrackingPair> & ExactPointCheck::trackingPairs(std::size_t frequency, std::size_t bound)
{
    TrackingPairs & cached = m_pairs[frequency];
    const std::vector<std::size_t> & working = m_working[frequency];
    if (cached.workingSize == working.size()) {
        return cached.pairs;
    }

    const FrequencyReference & at = m_reference->frequencies()[frequency];
    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const double d2 = trackingFactor(at.bounds[bound].boundDb) + 1.0;

    cached.pairs.clear();
    cached.pairs.reserve(m_hull[frequency].size() * working.size());
    for (const std::size_t n : m_hull[frequency]) {
        for (const std::size_t j : working) {
            const std::complex<double> w = d2 * q[j] - q[n];
            const double angle = std::arg(w);
            cached.pairs.push_back({std::abs(w), std::cos(angle), std::sin(angle), d2 * std::norm(q[j]) - std::norm(q[n])});
        }
    }
    cached.workingSize = working.size();
    return cached.pairs;
}

}
