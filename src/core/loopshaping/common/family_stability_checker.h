#ifndef QFTBX_FAMILY_STABILITY_CHECKER_H
#define QFTBX_FAMILY_STABILITY_CHECKER_H

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/parameter_grids.h"

/**
 * @file
 * @brief Whether a candidate controller leaves every plant of the swept
 * family asymptotically stable in closed loop.
 *
 * The QFT bounds are imposed at a handful of design frequencies chosen by
 * the engineer, and satisfying them there does not make the closed loop
 * stable: the crossing that decides it can fall between two of them. That
 * is why a search can return, and did return, a design that meets every
 * bound and leaves a third of the family unstable.
 *
 * This closes the loop with every member of the family the templates were
 * swept over and asks whether the characteristic polynomial
 * \f$ N_P N_C + D_P D_C \f$ is Hurwitz. It adds no frequency to the problem
 * the engineer posed - it is exact algebra on the plants already in hand -
 * so it belongs inside the search, as the last thing asked of a candidate
 * before it is returned. Validating a design against a denser set of
 * requirements is a different job and a later step.
 *
 * The plants' polynomials are built once, when the checker is; a verdict is
 * then a polynomial product and a Routh table per member, tens of
 * microseconds for a family of hundreds, and it stops at the first member
 * that fails. A plant or controller with a delay, one that is not a
 * rational function, or a project with no record of its sweep leaves the
 * checker unusable, and the caller then goes on as it did before: the
 * checker never approves what it cannot decide.
 *
 * It keeps a clone of the controller structure, since the search gives its
 * own away to the first box of the list.
 */
namespace qftbx {

class FamilyStabilityChecker
{
public:
    FamilyStabilityChecker(LtiSystem * plant, LtiSystem * controller, const ParameterGrids & sweep);

    bool usable() const { return m_usable; }
    std::size_t members() const { return m_members.size(); }

    bool isStable(const PointController & point);

    struct Statistics {
        std::size_t verdicts = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    std::unique_ptr<LtiSystem> m_controller;
    std::vector<LtiSystem::Polynomials> m_members;
    bool m_usable = false;
    Statistics m_statistics;
};

}

#endif
