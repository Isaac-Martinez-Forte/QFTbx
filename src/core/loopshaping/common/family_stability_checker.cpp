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

#include <optional>
#include <vector>

#include "src/core/loopshaping/common/specification_checker.h"
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
            return false;
        }
    }

    return true;
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
