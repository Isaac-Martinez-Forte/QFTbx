/**
 * @file
 * @brief Closing the loop with every plant of the sweep, by the Routh table
 * and, to confirm, by the roots, and proving a whole box of controllers
 * unstable with a plant of the sweep or with the nominal plant.
 *
 * A verdict multiplies every member's numerator and denominator by the
 * candidate's and asks whether the sum is Hurwitz, and the nominal verdict
 * the same of the nominal plant; the confirmation asks the verifier's
 * criterion the same question of the nominal plant and of every member. A box proof encloses the
 * box's factors in intervals and asks the interval Routh table, bisecting
 * the gain, or every parameter at the nominal plant, until it decides.
 */

#include "src/core/loopshaping/common/family_stability_checker.h"

#include <algorithm>
#include <cmath>
#include <functional>
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

struct BoxIntervals {
    LtiSystem::Polynomials corner;
    LtiSystem::SystemType type = LtiSystem::SystemType::ZeroPoleGain;
    std::vector<Interval> zeros;
    std::vector<Interval> poles;
    Interval gain;
};

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
    return intervals;
}

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

double relativeWidth(const Interval & x)
{
    const double scale = std::max(std::abs(x.lower()), std::abs(x.upper()));
    return scale > 0.0 ? (x.upper() - x.lower()) / scale : 0.0;
}

bool provenUnstableWith(const LtiSystem::Polynomials & plant, const BoxIntervals & box, Bisect bisect)
{
    if (math::isHurwitz(characteristicOf(plant, box.corner))) {
        return false;
    }

    const std::vector<Interval> plantNumerator = asIntervals(plant.numerator);
    const std::vector<Interval> plantDenominator = asIntervals(plant.denominator);

    std::vector<Interval> zeros = box.zeros;
    std::vector<Interval> poles = box.poles;
    Interval gain = box.gain;

    const std::function<bool (int)> provenOn = [&](int depth) {
        std::vector<Interval> perGain = math::intervalPolynomialProduct(plantNumerator, factorProductOf(zeros, box.type));
        for (Interval & c : perGain) {
            c = c * gain;
        }
        const std::vector<Interval> withoutGain =
                math::intervalPolynomialProduct(plantDenominator, factorProductOf(poles, box.type));
        if (math::provablyNotHurwitz(math::intervalPolynomialSum(withoutGain, perGain))) {
            return true;
        }
        if (depth >= kMaxBisectionDepth) {
            return false;
        }

        Interval * widest = &gain;
        if (bisect == Bisect::EveryParameter) {
            for (std::vector<Interval> * roots : {&zeros, &poles}) {
                for (Interval & r : *roots) {
                    if (relativeWidth(r) > relativeWidth(*widest)) {
                        widest = &r;
                    }
                }
            }
        }
        if (!(relativeWidth(*widest) > kNegligibleWidth)) {
            return false;
        }

        const Interval whole = *widest;
        const double middle = 0.5 * (whole.lower() + whole.upper());
        *widest = Interval(whole.lower(), middle);
        const bool lower = provenOn(depth + 1);
        *widest = Interval(middle, whole.upper());
        const bool proven = lower && provenOn(depth + 1);
        *widest = whole;
        return proven;
    };

    return provenOn(0);
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
