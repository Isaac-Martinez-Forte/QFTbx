/**
 * @file
 * @brief One candidate of the search against the specifications themselves,
 * and the exact gain set along the ray of its phase.
 */

#include "src/core/loopshaping/common/exact_point_check.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

#include "src/core/common/exception.h"
#include "src/core/math/convex_hull.h"
#include "src/core/math/line_envelope.h"
#include "src/core/math/quadratic_set.h"

namespace qftbx {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();
constexpr double kLn10 = 2.302585092994045684;
constexpr double kPi = 3.14159265358979323846;
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

RangeUnion fromParts(const std::vector<double> & lower, const std::vector<double> & upper)
{
    return RangeUnion::of(lower.data(), upper.data(), lower.size());
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

    m_byOmega.assign(omega.size(), std::numeric_limits<std::size_t>::max());
    for (std::size_t f = 0; f < m_reference->frequencies().size(); ++f) {
        const FrequencyReference & at = m_reference->frequencies()[f];
        m_byOmega[at.index] = f;
        std::vector<std::size_t> seed = math::convexHullVertices(at.nominalOverValueSet);
        if (seed.empty()) {
            seed.resize(at.nominalOverValueSet.size());
            std::iota(seed.begin(), seed.end(), std::size_t(0));
        }
        std::sort(seed.begin(), seed.end());
        m_hull.push_back(seed);
        m_statistics.largestWorkingSet = std::max(m_statistics.largestWorkingSet, seed.size());
        m_working.push_back(std::move(seed));
    }
}

namespace {

double largestCosineOver(double theta, double from, double to)
{
    const double twoPi = 2.0 * kPi;
    if (to - from >= twoPi) {
        return 1.0;
    }
    double shifted = std::fmod(theta - from, twoPi);
    if (shifted < 0.0) {
        shifted += twoPi;
    }
    if (shifted <= to - from) {
        return 1.0;
    }
    return std::max(std::cos(from - theta), std::cos(to - theta));
}

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

void forbiddenOf(double a, double b, double c, std::vector<double> & lower, std::vector<double> & upper)
{
    if (a == 0.0) {
        if (b == 0.0) {
            if (c < 0.0) {
                lower.push_back(0.0);
                upper.push_back(kInfinity);
            }
            return;
        }
        const double root = -c / b;
        if (b > 0.0) {
            if (root > 0.0) {
                lower.push_back(0.0);
                upper.push_back(root);
            }
        } else {
            lower.push_back(std::max(0.0, root));
            upper.push_back(kInfinity);
        }
        return;
    }

    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) {
        if (a < 0.0) {
            lower.push_back(0.0);
            upper.push_back(kInfinity);
        }
        return;
    }
    const double s = std::sqrt(discriminant);
    const double qv = -0.5 * (b + (b >= 0.0 ? s : -s));
    double r1 = 0.0, r2 = 0.0;
    if (qv != 0.0) {
        r1 = qv / a;
        r2 = c / qv;
    }
    if (r1 > r2) {
        std::swap(r1, r2);
    }
    if (a > 0.0) {
        if (r2 > 0.0) {
            lower.push_back(std::max(0.0, r1));
            upper.push_back(r2);
        }
    } else {
        if (r1 > 0.0) {
            lower.push_back(0.0);
            upper.push_back(r1);
        }
        lower.push_back(std::max(0.0, r2));
        upper.push_back(kInfinity);
    }
}

}

ExactPointCheck::SectorVerdict ExactPointCheck::sectorVerdict(std::size_t frequency, Range phaseDegrees,
                                                              Range magnitudeDb)
{
    requireUsable();

    SectorVerdict verdict;
    if (frequency >= m_byOmega.size() || m_byOmega[frequency] == std::numeric_limits<std::size_t>::max()) {
        return verdict;
    }
    ++m_statistics.sectorVerdicts;

    const std::size_t f = m_byOmega[frequency];
    const FrequencyReference & at = m_reference->frequencies()[f];
    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const double from = phaseDegrees.min * kPi / 180.0;
    const double to = phaseDegrees.max * kPi / 180.0;
    const double g1 = std::pow(10.0, magnitudeDb.min / 20.0);
    const double g2 = std::pow(10.0, magnitudeDb.max / 20.0);

    std::vector<double> & lower = m_forbiddenLower;
    std::vector<double> & upper = m_forbiddenUpper;
    lower.clear();
    upper.clear();

    for (const FrequencyReference::Bound & bound : at.bounds) {
        if (bound.type == SpecificationType::TrackingLower) {
            const double dm1 = std::expm1(bound.boundDb * kLn10 / 10.0);
            const double d2 = dm1 + 1.0;
            for (const std::size_t n : m_hull[f]) {
                for (const std::size_t j : m_working[f]) {
                    const std::complex<double> w = d2 * q[j] - q[n];
                    const double rho = std::abs(w) * largestCosineOver(std::arg(w), from, to);
                    const double a = dm1, b = 2.0 * rho, c = d2 * std::norm(q[j]) - std::norm(q[n]);
                    if (largestOver(a, b, c, g1, g2) < 0.0) {
                        verdict.provablyInfeasible = true;
                    }
                    forbiddenOf(a, b, c, lower, upper);
                }
            }
            continue;
        }

        const double W = std::pow(10.0, bound.boundDb / 20.0);
        for (std::size_t n = 0; n < q.size(); ++n) {
            double s = 0.0, t = 0.0;
            switch (bound.type) {
            case SpecificationType::Stability:
            case SpecificationType::SensorNoise:
                s = 1.0 / W;
                break;
            case SpecificationType::OutputDisturbance:
                t = std::abs(q[n]) / W;
                break;
            case SpecificationType::InputDisturbance:
                t = std::abs(at.nominalPlant) / W;
                break;
            case SpecificationType::ControlEffort: {
                const double plant = std::abs((*at.valueSet)[n]);
                if (!(plant > 0.0)) {
                    continue;
                }
                s = 1.0 / (W * plant);
                break;
            }
            case SpecificationType::TrackingLower:
            case SpecificationType::TrackingUpper:
                continue;
            }
            const double cmax = std::abs(q[n]) * largestCosineOver(std::arg(q[n]), from, to);
            const double a = 1.0 - s * s, b = 2.0 * (cmax - s * t), c = std::norm(q[n]) - t * t;
            if (largestOver(a, b, c, g1, g2) < 0.0) {
                verdict.provablyInfeasible = true;
            }
            forbiddenOf(a, b, c, lower, upper);
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
            verdict.forbiddenBelowDb = covered < kInfinity ? 20.0 * std::log10(covered) : kInfinity;
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
            verdict.forbiddenAboveDb = reached > 0.0 ? 20.0 * std::log10(reached) : -kInfinity;
        }
    }

    if (verdict.forbiddenBelowDb >= magnitudeDb.max || verdict.forbiddenAboveDb <= magnitudeDb.min) {
        verdict.provablyInfeasible = true;
    }

    return verdict;
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

    ++m_statistics.verdicts;

    const std::vector<FrequencyReference> & frequencies = m_reference->frequencies();
    const std::size_t count = frequencies.size();

    for (std::size_t step = 0; step < count; ++step) {
        const std::size_t i = (m_firstToAsk + step) % count;
        ++m_statistics.kernelPasses;
        if (!(m_reference->worstExcessAt(frequencies[i], loopAt(frequencies[i], point)) <= -kToleranceDb)) {
            m_firstToAsk = i;
            ++m_statistics.rejections;
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
        m_reference->recordExcesses(at, loopAt(at, point), check);
    }
    return check;
}

RangeUnion ExactPointCheck::admissibleMagnitudes(const FrequencyReference & at, std::complex<double> direction,
                                                 const std::vector<std::size_t> & plants) const
{
    RangeUnion allowed = RangeUnion::of(0.0, kInfinity);

    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;
    const auto cosine = [&](std::size_t n) { return std::real(std::conj(q[n]) * direction); };

    for (const FrequencyReference::Bound & bound : at.bounds) {
        if (allowed.isEmpty()) {
            break;
        }

        const double boundDb = bound.boundDb - kToleranceDb;

        if (bound.type == SpecificationType::TrackingLower) {
            const double dm1 = std::expm1(boundDb * kLn10 / 10.0);
            const double d2 = dm1 + 1.0;

            std::vector<math::Line> lines;
            lines.reserve(plants.size());
            for (const std::size_t n : plants) {
                lines.push_back({2.0 * cosine(n), std::norm(q[n]), n});
            }
            const std::vector<math::EnvelopePiece> nearest = math::lowerEnvelope(lines);
            const std::vector<math::EnvelopePiece> farthest = math::upperEnvelope(lines);

            std::vector<double> lower, upper;
            std::size_t i = 0, j = 0;
            double from = 0.0;
            while (i < nearest.size() && j < farthest.size()) {
                const double nextNearest = i + 1 < nearest.size() ? nearest[i + 1].from : kInfinity;
                const double nextFarthest = j + 1 < farthest.size() ? farthest[j + 1].from : kInfinity;
                const double to = std::min(nextNearest, nextFarthest);
                const std::size_t n = nearest[i].line;
                const std::size_t m = farthest[j].line;

                RangeUnion piece = math::whereNonNegative(dm1, 2.0 * (d2 * cosine(n) - cosine(m)),
                                                          d2 * std::norm(q[n]) - std::norm(q[m]));
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
            allowed.intersectWith(fromParts(lower, upper));
            continue;
        }

        const double W = std::pow(10.0, boundDb / 20.0);
        for (const std::size_t n : plants) {
            if (allowed.isEmpty()) {
                break;
            }
            const double c = cosine(n);
            const double q2 = std::norm(q[n]);
            double s = 0.0, t = 0.0;
            switch (bound.type) {
            case SpecificationType::Stability:
            case SpecificationType::SensorNoise:
                s = 1.0 / W;
                break;
            case SpecificationType::OutputDisturbance:
                t = std::sqrt(q2) / W;
                break;
            case SpecificationType::InputDisturbance:
                t = std::abs(at.nominalPlant) / W;
                break;
            case SpecificationType::ControlEffort: {
                const double plant = std::abs((*at.valueSet)[n]);
                if (!(plant > 0.0)) {
                    return RangeUnion();
                }
                s = 1.0 / (W * plant);
                break;
            }
            case SpecificationType::TrackingLower:
            case SpecificationType::TrackingUpper:
                continue;
            }
            allowed.intersectWith(math::whereNonNegative(1.0 - s * s, 2.0 * (c - s * t), q2 - t * t));
        }
    }

    return allowed;
}

RangeUnion ExactPointCheck::admissibleGainsDbOver(const std::vector<double> & zeros, const std::vector<double> & poles,
                                                  Range gainRange,
                                                  const std::vector<std::vector<std::size_t>> * workingSets) const
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

        std::vector<std::size_t> everyPlant;
        if (workingSets == nullptr) {
            everyPlant.resize(at.nominalOverValueSet.size());
            std::iota(everyPlant.begin(), everyPlant.end(), std::size_t(0));
        }
        const RangeUnion magnitudes = admissibleMagnitudes(at, unit / mu,
                                                           workingSets == nullptr ? everyPlant : (*workingSets)[f]);

        std::vector<double> lower, upper;
        for (const Range & part : magnitudes.components()) {
            lower.push_back(toDb(part.min) - muDb);
            upper.push_back(toDb(part.max) - muDb);
        }
        gains.intersectWith(fromParts(lower, upper));
    }

    return gains;
}

RangeUnion ExactPointCheck::admissibleGainsDb(const std::vector<double> & zeros, const std::vector<double> & poles,
                                              Range gainRange) const
{
    requireUsable();
    return admissibleGainsDbOver(zeros, poles, gainRange, nullptr);
}

bool ExactPointCheck::growWorkingSet(std::size_t frequency, const PointController & point)
{
    const FrequencyReference & at = m_reference->frequencies()[frequency];
    const std::complex<double> loop = loopAt(at, point);
    const std::vector<std::complex<double>> & q = at.nominalOverValueSet;

    bool outputInForce = false, effortInForce = false;
    for (const FrequencyReference::Bound & bound : at.bounds) {
        outputInForce = outputInForce || bound.type == SpecificationType::OutputDisturbance;
        effortInForce = effortInForce || bound.type == SpecificationType::ControlEffort;
    }

    std::size_t nearest = 0, farthest = 0, worstOutput = 0, worstEffort = 0;
    double dMin = kInfinity, dMax = -kInfinity, outputMax = -kInfinity, effortMax = -kInfinity;
    for (std::size_t n = 0; n < q.size(); ++n) {
        const double d = std::abs(q[n] + loop);
        if (d < dMin) { dMin = d; nearest = n; }
        if (d > dMax) { dMax = d; farthest = n; }
        if (outputInForce) {
            const double output = std::abs(q[n]) / d;
            if (output > outputMax) { outputMax = output; worstOutput = n; }
        }
        if (effortInForce) {
            const double effort = std::abs(loop) / (std::abs((*at.valueSet)[n]) * d);
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

    for (std::size_t round = 0; round < kMaxRounds; ++round) {
        result.rounds = round;
        const RangeUnion set = admissibleGainsDbOver(zeros, poles, gainRange, &m_working);

        bool grew = false;
        for (const Range & component : set.components()) {
            const double lo = std::pow(10.0, component.min / 20.0);
            const double hi = std::pow(10.0, component.max / 20.0);
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
                if (admits(candidate)) {
                    result.gain = gain;
                    m_statistics.exchangeRounds += result.rounds;
                    m_statistics.ladderSteps += result.ladderSteps;
                    return result;
                }
                if (growWorkingSet(m_firstToAsk, candidate)) {
                    grew = true;
                    break;
                }
                ++result.ladderSteps;
            }
            if (grew) {
                break;
            }
        }

        if (!grew) {
            break;
        }
    }

    m_statistics.exchangeRounds += result.rounds;
    m_statistics.ladderSteps += result.ladderSteps;
    return result;
}

}
