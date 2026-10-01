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
#include <cmath>
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

    if (m_usable) {
        m_nominal = nominalPolynomials(*plant);
    }
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
        const std::vector<double> characteristic = characteristicOf(plant, *loop);
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

bool provenUnstableWith(const LtiSystem::Polynomials & plant, const BoxIntervals & box, bool splitRoots)
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
        if (depth >= 12) {
            return false;
        }

        Interval * widest = &gain;
        if (splitRoots) {
            for (std::vector<Interval> * roots : {&zeros, &poles}) {
                for (Interval & r : *roots) {
                    if (relativeWidth(r) > relativeWidth(*widest)) {
                        widest = &r;
                    }
                }
            }
        }
        if (!(relativeWidth(*widest) > 1e-9)) {
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

bool FamilyStabilityChecker::isBoxUnstable(LtiSystem * box)
{
    if (!m_usable || box == nullptr) {
        return false;
    }
    const std::optional<BoxIntervals> intervals = boxIntervalsOf(box, *m_controller);
    if (!intervals.has_value()) {
        return false;
    }

    ++m_statistics.boxVerdicts;

    for (const std::size_t member : m_recentRefusers) {
        if (provenUnstableWith(m_family.member(member), *intervals, false)) {
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

    if (provenUnstableWith(*m_nominal, *intervals, true)) {
        ++m_statistics.nominalBoxPrunes;
        return true;
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
