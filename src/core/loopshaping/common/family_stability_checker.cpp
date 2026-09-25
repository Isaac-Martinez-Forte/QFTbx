/**
 * @file
 * @brief Closing the loop with every plant of the sweep, by the Routh table
 * and, to confirm, by the roots.
 *
 * A verdict multiplies every member's numerator and denominator by the
 * candidate's and asks whether the sum is Hurwitz; the confirmation asks
 * the verifier's family criterion the same question.
 */

#include "src/core/loopshaping/common/family_stability_checker.h"

#include "src/core/loopshaping/common/point_controller.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>

#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/interval_polynomial.h"
#include "src/core/math/polynomial.h"

namespace qftbx {

FamilyStabilityChecker::FamilyStabilityChecker(LtiSystem * plant, LtiSystem * controller,
                                               const ParameterGrids & sweep)
{
    if (plant == nullptr || controller == nullptr || hasDelay(*controller)) {
        return;
    }

    m_controller = controller->clone();
    m_family = SweptFamily(*plant, sweep);
    m_usable = m_family.usable();
}

void FamilyStabilityChecker::rememberRefuser(std::size_t member)
{
    const auto known = std::find(m_recentRefusers.begin(), m_recentRefusers.end(), member);
    if (known != m_recentRefusers.end()) {
        m_recentRefusers.erase(known);
    }
    m_recentRefusers.insert(m_recentRefusers.begin(), member);
    if (m_recentRefusers.size() > 4) {
        m_recentRefusers.pop_back();
    }
}

bool FamilyStabilityChecker::isStable(const PointController & point)
{
    if (!m_usable) {
        return true;
    }

    ++m_statistics.verdicts;

    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return true;
    }

    const std::size_t count = m_family.size();
    for (std::size_t step = 0; step < count; ++step) {
        const std::size_t member = (m_lastUnstable + step) % count;
        const LtiSystem::Polynomials & plant = m_family.member(member);
        const std::vector<double> characteristic =
                math::polynomialSum(math::polynomialProduct(plant.numerator, loop->numerator),
                                    math::polynomialProduct(plant.denominator, loop->denominator));
        if (!math::isHurwitz(characteristic)) {
            m_lastUnstable = member;
            rememberRefuser(member);
            return false;
        }
    }

    return true;
}

namespace {

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

}

bool FamilyStabilityChecker::isBoxUnstable(LtiSystem * box)
{
    if (!m_usable || box == nullptr) {
        return false;
    }
    const LtiSystem::SystemType type = box->type();
    if (type != LtiSystem::SystemType::ZeroPoleGain && type != LtiSystem::SystemType::TimeConstantGain) {
        return false;
    }

    ++m_statistics.boxVerdicts;

    const PointController corner = cornerOf(box, true);
    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(corner.zeros, corner.poles, corner.gain);
    if (!loop.has_value()) {
        return false;
    }

    std::vector<Interval> numerator{Interval(1.0)};
    std::vector<Interval> denominator{Interval(1.0)};
    for (const Parameter & zero : box->numerator()) {
        const Interval z = intervalOf(zero);
        if (type == LtiSystem::SystemType::ZeroPoleGain) {
            numerator = math::intervalPolynomialProduct(numerator, {Interval(1.0), z});
        } else {
            if (z.containsZero()) {
                return false;
            }
            numerator = math::intervalPolynomialProduct(numerator, {Interval(1.0) / z, Interval(1.0)});
        }
    }
    for (const Parameter & pole : box->denominator()) {
        const Interval p = intervalOf(pole);
        if (type == LtiSystem::SystemType::ZeroPoleGain) {
            denominator = math::intervalPolynomialProduct(denominator, {Interval(1.0), p});
        } else {
            if (p.containsZero()) {
                return false;
            }
            denominator = math::intervalPolynomialProduct(denominator, {Interval(1.0) / p, Interval(1.0)});
        }
    }
    const Interval gain = intervalOf(box->gain());

    for (const std::size_t member : m_recentRefusers) {
        const LtiSystem::Polynomials & plant = m_family.member(member);

        if (math::isHurwitz(math::polynomialSum(math::polynomialProduct(plant.numerator, loop->numerator),
                                                math::polynomialProduct(plant.denominator, loop->denominator)))) {
            continue;
        }

        const std::vector<Interval> withoutGain =
                math::intervalPolynomialProduct(asIntervals(plant.denominator), denominator);
        const std::vector<Interval> perGain =
                math::intervalPolynomialProduct(asIntervals(plant.numerator), numerator);

        const auto unstableFor = [&](const Interval & k) {
            std::vector<Interval> scaled = perGain;
            for (Interval & c : scaled) {
                c = c * k;
            }
            return math::provablyNotHurwitz(math::intervalPolynomialSum(withoutGain, scaled));
        };
        const std::function<bool (double, double, int)> provenOn = [&](double lower, double upper, int depth) {
            if (unstableFor(Interval(lower, upper))) {
                return true;
            }
            if (depth >= 12 || !(upper - lower > 1e-9 * upper)) {
                return false;
            }
            const double middle = 0.5 * (lower + upper);
            return provenOn(lower, middle, depth + 1) && provenOn(middle, upper, depth + 1);
        };

        if (provenOn(gain.lower(), gain.upper(), 0)) {
            ++m_statistics.boxPrunes;
            return true;
        }
    }

    return false;
}

bool FamilyStabilityChecker::isStableByRoots(const PointController & point)
{
    if (!m_usable) {
        return true;
    }

    ++m_statistics.rootVerdicts;

    const std::optional<LtiSystem::Polynomials> loop =
            m_controller->polynomialsAt(point.zeros, point.poles, point.gain);
    if (!loop.has_value()) {
        return true;
    }

    return familyStabilityAt(m_family, *loop).unstableMembers == 0;
}

}
