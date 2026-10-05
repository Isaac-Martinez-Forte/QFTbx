/**
 * @file
 * @brief Closing the loop with every plant of the sweep, by the Routh table
 * and, to confirm, by the roots, and proving a whole box of controllers
 * unstable with a plant of the sweep or with the nominal plant.
 *
 * A verdict multiplies every member's numerator and denominator by the
 * candidate's and asks whether the sum is Hurwitz, and the nominal verdict
 * the same of the nominal plant; the confirmation asks the verifier's
 * criterion the same question of the nominal plant and of every member. A
 * box proof encloses the box's factors in intervals and asks the interval
 * Routh table, bisecting the gain, or every parameter at the nominal plant,
 * until it decides; the products of the factors are formed once per box and
 * carried down the bisection, and only the side of a root that was split is
 * formed again.
 */

#include "src/core/loopshaping/common/family_stability_checker.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/interval_polynomial.h"
#include "src/core/math/polynomial.h"

namespace qftbx {

namespace {

constexpr std::size_t kRecentRefusers = 4;
constexpr int kMaxBisectionDepth = 12;
constexpr double kNegligibleWidth = 1e-9;
constexpr std::size_t kWorkingPlants = 8;
constexpr std::size_t kExchangeEvery = 16;
constexpr double kShaveResolutionDecades = 0.005;
constexpr std::size_t kNominalMember = std::numeric_limits<std::size_t>::max();

enum class Bisect { GainOnly, EveryParameter };

Interval intervalOf(const Parameter & parameter)
{
    return parameter.isUncertain() ? Interval(parameter.range().min, parameter.range().max)
                                   : Interval(parameter.nominal());
}

std::vector<Interval> asIntervals(const std::vector<double> & coefficients)
{
    std::vector<Interval> intervals;
    intervals.reserve(coefficients.size());
    for (const double c : coefficients) {
        intervals.emplace_back(c);
    }
    return intervals;
}

enum class Split { Gain, Zero, Pole };

struct BoxIntervals {
    LtiSystem::Polynomials corner;
    LtiSystem::SystemType type = LtiSystem::SystemType::ZeroPoleGain;
    std::vector<Interval> zeros;
    std::vector<Interval> poles;
    Interval gain;
    std::vector<Interval> zeroFactors;
    std::vector<Interval> poleFactors;
};

std::vector<Interval> factorProductOf(const std::vector<Interval> & roots, LtiSystem::SystemType type)
{
    std::vector<Interval> product{Interval(1.0)};
    for (const Interval & r : roots) {
        product = math::intervalPolynomialProduct(
                    product, type == LtiSystem::SystemType::ZeroPoleGain ? std::vector<Interval>{Interval(1.0), r}
                                                                          : std::vector<Interval>{Interval(1.0) / r, Interval(1.0)});
    }
    return product;
}

std::optional<BoxIntervals> boxIntervalsOf(LtiSystem * box, LtiSystem & controller)
{
    const LtiSystem::SystemType type = box->type();
    if (type != LtiSystem::SystemType::ZeroPoleGain && type != LtiSystem::SystemType::TimeConstantGain) {
        return std::nullopt;
    }

    const PointController corner = cornerOf(box, true);
    const std::optional<LtiSystem::Polynomials> loop = controller.polynomialsAt(corner.zeros, corner.poles, corner.gain);
    if (!loop.has_value()) {
        return std::nullopt;
    }

    BoxIntervals intervals;
    intervals.corner = *loop;
    intervals.type = type;
    for (const Parameter & zero : box->numerator()) {
        intervals.zeros.push_back(intervalOf(zero));
    }
    for (const Parameter & pole : box->denominator()) {
        intervals.poles.push_back(intervalOf(pole));
    }
    if (type == LtiSystem::SystemType::TimeConstantGain) {
        for (const std::vector<Interval> * roots : {&intervals.zeros, &intervals.poles}) {
            for (const Interval & r : *roots) {
                if (r.containsZero()) {
                    return std::nullopt;
                }
            }
        }
    }
    intervals.gain = intervalOf(box->gain());
    intervals.zeroFactors = factorProductOf(intervals.zeros, type);
    intervals.poleFactors = factorProductOf(intervals.poles, type);
    return intervals;
}


double centreOf(const Interval & x)
{
    return x.lower() > 0.0 ? std::sqrt(x.lower() * x.upper()) : x.midpoint();
}

template <class Proven>
std::optional<Range> shaved(Range gains, Proven && provenOn)
{
    if (provenOn(gains.min, gains.max)) {
        return std::nullopt;
    }
    if (!(gains.min > 0.0) || !(gains.min < gains.max)) {
        return gains;
    }

    double top = gains.max;
    double step = std::log10(gains.max / gains.min) / 2.0;
    while (step >= kShaveResolutionDecades && top > gains.min) {
        const double lower = std::max(gains.min, top / std::pow(10.0, step));
        if (provenOn(lower, top)) {
            top = lower;
            step *= 2.0;
        } else {
            step /= 2.0;
        }
    }
    if (!(top > gains.min)) {
        return std::nullopt;
    }

    double bottom = gains.min;
    step = std::log10(top / gains.min) / 2.0;
    while (step >= kShaveResolutionDecades && bottom < top) {
        const double upper = std::min(top, bottom * std::pow(10.0, step));
        if (provenOn(bottom, upper)) {
            bottom = upper;
            step *= 2.0;
        } else {
            step /= 2.0;
        }
    }
    if (!(bottom < top)) {
        return std::nullopt;
    }
    return Range(bottom, top);
}

double relativeWidth(const Interval & x)
{
    const double scale = std::max(std::abs(x.lower()), std::abs(x.upper()));
    return scale > 0.0 ? (x.upper() - x.lower()) / scale : 0.0;
}

struct ProofSearch {
    const BoxIntervals & box;
    Bisect bisect;
    const std::vector<Interval> & plantNumerator;
    const std::vector<Interval> & plantDenominator;
    std::vector<Interval> zeros;
    std::vector<Interval> poles;
    Interval gain;

    bool provenOn(const std::vector<Interval> & perUnitGain, const std::vector<Interval> & withoutGain, int depth)
    {
        std::vector<Interval> perGain = perUnitGain;
        for (Interval & c : perGain) {
            c = c * gain;
        }
        if (math::provablyNotHurwitz(math::intervalPolynomialSum(withoutGain, perGain))) {
            return true;
        }
        if (depth >= kMaxBisectionDepth) {
            return false;
        }

        Interval * widest = &gain;
        Split split = Split::Gain;
        if (bisect == Bisect::EveryParameter) {
            for (Interval & r : zeros) {
                if (relativeWidth(r) > relativeWidth(*widest)) {
                    widest = &r;
                    split = Split::Zero;
                }
            }
            for (Interval & r : poles) {
                if (relativeWidth(r) > relativeWidth(*widest)) {
                    widest = &r;
                    split = Split::Pole;
                }
            }
        }
        if (!(relativeWidth(*widest) > kNegligibleWidth)) {
            return false;
        }

        const auto half = [&] {
            switch (split) {
            case Split::Zero:
                return provenOn(math::intervalPolynomialProduct(plantNumerator, factorProductOf(zeros, box.type)),
                                withoutGain, depth + 1);
            case Split::Pole:
                return provenOn(perUnitGain,
                                math::intervalPolynomialProduct(plantDenominator, factorProductOf(poles, box.type)),
                                depth + 1);
            case Split::Gain:
                break;
            }
            return provenOn(perUnitGain, withoutGain, depth + 1);
        };

        const Interval whole = *widest;
        const double middle = 0.5 * (whole.lower() + whole.upper());
        *widest = Interval(whole.lower(), middle);
        const bool lower = half();
        *widest = Interval(middle, whole.upper());
        const bool proven = lower && half();
        *widest = whole;
        return proven;
    }
};

bool provenUnstableWith(const LtiSystem::Polynomials & plant, const BoxIntervals & box, Bisect bisect)
{
    if (math::isHurwitz(characteristicOf(plant, box.corner))) {
        return false;
    }

    const std::vector<Interval> plantNumerator = asIntervals(plant.numerator);
    const std::vector<Interval> plantDenominator = asIntervals(plant.denominator);

    ProofSearch search{box, bisect, plantNumerator, plantDenominator, box.zeros, box.poles, box.gain};
    return search.provenOn(math::intervalPolynomialProduct(plantNumerator, box.zeroFactors),
                           math::intervalPolynomialProduct(plantDenominator, box.poleFactors), 0);
}

}

FamilyStabilityChecker::FamilyStabilityChecker(LtiSystem * plant, LtiSystem * controller,
                                               const ParameterGrids & sweep)
{
    if (plant == nullptr || controller == nullptr || hasDelay(*controller)) {
        return;
    }

    m_controller = controller->clone();
    m_family = SweptFamily(*plant, sweep);

    if (!hasDelay(*plant)) {
        m_nominal = nominalPolynomials(*plant);
    }
    if (m_nominal.has_value()) {
        m_working.push_back(kNominalMember);
    }
}

bool FamilyStabilityChecker::isStable(const PointController & point)
{
    if (!m_family.usable()) {
        return true;
    }

    ++m_statistics.verdicts;

    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return true;
    }

    const std::size_t count = m_family.size();
    const std::size_t first = m_recentRefusers.empty() ? 0 : m_recentRefusers.front();
    for (std::size_t step = 0; step < count; ++step) {
        const std::size_t member = (first + step) % count;
        const LtiSystem::Polynomials & plant = m_family.member(member);
        const std::vector<double> characteristic = characteristicOf(plant, *loop);
        if (!math::isHurwitz(characteristic)) {
            rememberRefuser(member);
            return false;
        }
    }

    return true;
}

bool FamilyStabilityChecker::isStableAtNominal(const PointController & point)
{
    if (!m_nominal.has_value()) {
        return true;
    }

    ++m_statistics.nominalVerdicts;

    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return true;
    }

    return math::isHurwitz(characteristicOf(*m_nominal, *loop));
}

bool FamilyStabilityChecker::isStableByRoots(const PointController & point)
{
    if (!m_family.usable() && !m_nominal.has_value()) {
        return true;
    }

    ++m_statistics.rootVerdicts;

    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return true;
    }

    if (m_nominal.has_value() && !rootVerdictOf(characteristicOf(*m_nominal, *loop)).stable) {
        return false;
    }
    return !m_family.usable() || familyStabilityAt(m_family, *loop).unstableMembers == 0;
}

bool FamilyStabilityChecker::isBoxUnstable(LtiSystem * box)
{
    if (!m_family.usable() || box == nullptr) {
        return false;
    }
    const std::optional<BoxIntervals> intervals = boxIntervalsOf(box, *m_controller);
    if (!intervals.has_value()) {
        return false;
    }

    ++m_statistics.boxVerdicts;

    for (const std::size_t member : m_recentRefusers) {
        if (provenUnstableWith(m_family.member(member), *intervals, Bisect::GainOnly)) {
            ++m_statistics.boxPrunes;
            return true;
        }
    }

    return false;
}

bool FamilyStabilityChecker::isBoxUnstableAtNominal(LtiSystem * box)
{
    if (!m_nominal.has_value() || box == nullptr) {
        return false;
    }
    const std::optional<BoxIntervals> intervals = boxIntervalsOf(box, *m_controller);
    if (!intervals.has_value()) {
        return false;
    }

    ++m_statistics.nominalBoxVerdicts;

    if (provenUnstableWith(*m_nominal, *intervals, Bisect::EveryParameter)) {
        ++m_statistics.nominalBoxPrunes;
        return true;
    }
    return false;
}

std::optional<Range> FamilyStabilityChecker::shaveUnstableGains(LtiSystem * box, Range gains)
{
    if (box == nullptr || (m_working.empty() && m_recentRefusers.empty())) {
        return gains;
    }
    const std::optional<BoxIntervals> intervals = boxIntervalsOf(box, *m_controller);
    if (!intervals.has_value()) {
        return gains;
    }

    ++m_statistics.gainShaves;
    for (auto refuser = m_recentRefusers.rbegin(); refuser != m_recentRefusers.rend(); ++refuser) {
        if (std::find(m_working.begin(), m_working.end(), *refuser) == m_working.end()) {
            addWorkingPlant(*refuser);
        }
    }

    Range left = gains;
    for (int round = 0; round < 2; ++round) {
        struct Closed {
            std::vector<Interval> perUnitGain;
            std::vector<Interval> withoutGain;
        };
        std::vector<Closed> closed;
        closed.reserve(m_working.size());
        for (const std::size_t member : m_working) {
            const LtiSystem::Polynomials & plant = workingPlant(member);
            closed.push_back({math::intervalPolynomialProduct(asIntervals(plant.numerator), intervals->zeroFactors),
                              math::intervalPolynomialProduct(asIntervals(plant.denominator), intervals->poleFactors)});
        }
        const auto proven = [&](double a, double b) {
            const Interval gain(a, b);
            for (const Closed & loop : closed) {
                std::vector<Interval> perGain = loop.perUnitGain;
                for (Interval & coefficient : perGain) {
                    coefficient = coefficient * gain;
                }
                if (math::provablyNotHurwitz(math::intervalPolynomialSum(loop.withoutGain, perGain))) {
                    ++m_statistics.gainPiecesProven;
                    return true;
                }
            }
            return false;
        };
        const std::optional<Range> shavedGains = shaved(left, proven);
        if (!shavedGains.has_value()) {
            return std::nullopt;
        }
        left = *shavedGains;

        if (round == 1 || (m_shavesAsked++ % kExchangeEvery) != 0) {
            break;
        }
        PointController centre;
        centre.gain = left.max;
        for (const Interval & z : intervals->zeros) centre.zeros.push_back(centreOf(z));
        for (const Interval & p : intervals->poles) centre.poles.push_back(centreOf(p));
        const std::optional<std::size_t> refuser = refuserOutsideWorkingSet(centre);
        if (!refuser.has_value()) {
            break;
        }
        ++m_statistics.workingSetExchanges;
        addWorkingPlant(*refuser);
    }
    return left;
}

const LtiSystem::Polynomials & FamilyStabilityChecker::workingPlant(std::size_t member) const
{
    return member == kNominalMember ? *m_nominal : m_family.member(member);
}

void FamilyStabilityChecker::addWorkingPlant(std::size_t member)
{
    const auto known = std::find(m_working.begin(), m_working.end(), member);
    if (known != m_working.end()) {
        m_working.erase(known);
    }
    m_working.insert(m_working.begin(), member);
    if (m_working.size() > kWorkingPlants) {
        m_working.pop_back();
    }
}

std::optional<std::size_t> FamilyStabilityChecker::refuserOutsideWorkingSet(const PointController & point)
{
    const std::optional<LtiSystem::Polynomials> loop = m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return std::nullopt;
    }
    const auto outside = [&](std::size_t member) {
        return std::find(m_working.begin(), m_working.end(), member) == m_working.end();
    };
    if (m_nominal.has_value() && outside(kNominalMember) && !math::isHurwitz(characteristicOf(*m_nominal, *loop))) {
        return kNominalMember;
    }
    if (m_family.usable()) {
        for (std::size_t member = 0; member < m_family.size(); ++member) {
            if (outside(member) && !math::isHurwitz(characteristicOf(m_family.member(member), *loop))) {
                return member;
            }
        }
    }
    return std::nullopt;
}

void FamilyStabilityChecker::rememberRefuser(std::size_t member)
{
    const auto known = std::find(m_recentRefusers.begin(), m_recentRefusers.end(), member);
    if (known != m_recentRefusers.end()) {
        m_recentRefusers.erase(known);
    }
    m_recentRefusers.insert(m_recentRefusers.begin(), member);
    if (m_recentRefusers.size() > kRecentRefusers) {
        m_recentRefusers.pop_back();
    }
}

}
