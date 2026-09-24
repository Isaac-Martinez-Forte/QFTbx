#ifndef QFTBX_FAMILY_STABILITY_CHECKER_H
#define QFTBX_FAMILY_STABILITY_CHECKER_H

#include <cstddef>
#include <memory>

#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/swept_family.h"
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
 * The plants' polynomials are the SweptFamily's, built once, when the
 * checker is. isStable is the Routh table per member, tens of microseconds
 * for a family of hundreds, stopping at the first member that fails.
 * isStableByRoots is the verifier's own criterion on the same members, the
 * roots of every characteristic polynomial with the tolerance the verifier
 * applies to the axis, milliseconds rather than microseconds: the two agree
 * except in that tolerance band, and a candidate that is to be returned
 * under the exact reading is confirmed by the second so that the verifier
 * cannot refuse afterwards what the search accepted. A plant or controller
 * with a delay, one that is not a rational function, or a project with no
 * record of its sweep leaves the checker unusable, and the caller then goes
 * on as it did before: the checker never approves what it cannot decide.
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
    std::size_t members() const { return m_family.size(); }

    bool isStable(const PointController & point);

    bool isStableByRoots(const PointController & point);

    struct Statistics {
        std::size_t verdicts = 0;
        std::size_t rootVerdicts = 0;
    };
    const Statistics & statistics() const { return m_statistics; }

private:
    std::unique_ptr<LtiSystem> m_controller;
    SweptFamily m_family;
    bool m_usable = false;
    Statistics m_statistics;
};

}

#endif
