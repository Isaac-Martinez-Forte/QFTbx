#ifndef QFTBX_SPECIFICATION_CHECKER_H
#define QFTBX_SPECIFICATION_CHECKER_H

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "src/core/specifications/specification.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/parameter_grids.h"

/**
 * @file
 * @brief A returned controller checked against the specifications
 * themselves, not against the boundaries computed from them.
 *
 * Every loop-shaping algorithm searches against the boundaries: the
 * specifications mapped onto a Nichols grid, read off a template that is
 * itself a sample of the plant family. Each of those steps approximates, and
 * the approximations do not all err on the safe side: the phase grid is
 * read at its nearest node, and a sampled template underestimates the
 * family's worst case. So a controller the search certifies against the
 * boundaries can still violate the specification it was meant to satisfy.
 *
 * This checker closes that gap by construction. It takes the controller
 * and evaluates the closed-loop magnitudes at its own loop value, at every
 * design frequency, over the whole value set given - the full template, not
 * its contour - and compares them with the specification bounds at that
 * frequency. No grid, no column, no interpolation: the only approximation
 * left is the sampling of the plant family, which is the input. The result
 * is the excess over each active bound, in decibels, so that a positive
 * number is a violation and its size is the size of the violation.
 *
 * The specifications are sampled in frequency, and a loop that meets a
 * stability margin at every design frequency can still be closed-loop
 * unstable for some member of the family: the crossing that decides it can
 * fall between two design frequencies. So the checker also closes the loop
 * with every plant of the sweep the templates came from and finds the roots
 * of its characteristic polynomial, N_P N_C + D_P D_C, which is exact for
 * the sampled family and costs milliseconds. A loop with a delay or a plant
 * that is not a rational function has no such polynomial, and a project
 * whose templates came from a file without the sweep has no members to
 * walk: the check then says so instead of approving.
 *
 * It exists so that "the returned controller satisfies its specifications"
 * can be a test in the suite instead of a claim.
 *
 * The result lists every active specification at every design frequency with
 * the value achieved, the bound and the excess, and the largest excess. The
 * family part says how many plants were closed, how many are not
 * asymptotically stable - a closed-loop pole in the right half-plane, or on
 * the imaginary axis within 1e-7 of the largest pole, the tolerance the roots
 * are computed to - the largest real part over all of them and the plant it
 * belongs to, or why it could not be checked: no record of the sweep, a delay,
 * a plant that is not rational. A pole on the axis is not stability, and at
 * the floor of a gain box over a plant with poles of its own on the axis it is
 * what a minimum-gain search converges to. satisfied() asks for no excess and
 * no unstable plant. A parameter the sweep has no grid for stays at its
 * nominal value, and a frequency whose value set is empty contributes nothing.
 */
namespace qftbx {

struct SpecificationExcess
{
    std::size_t frequencyIndex = 0;
    double omega = 0.0;
    SpecificationType type = SpecificationType::Stability;
    double valueDb = 0.0;
    double boundDb = 0.0;
    double excessDb = 0.0;
};

struct FamilyStability
{
    enum class NotChecked { No, NoSweepRecord, Delay, NotRational };

    bool checked = false;
    NotChecked notChecked = NotChecked::NoSweepRecord;
    std::size_t members = 0;
    std::size_t unstableMembers = 0;
    double worstRealPart = -std::numeric_limits<double>::infinity();
    std::vector<std::pair<std::string, double>> worstMember;
};

struct SpecificationCheck
{
    std::vector<SpecificationExcess> entries;
    double worstExcessDb = -std::numeric_limits<double>::infinity();
    FamilyStability family;

    bool satisfied() const { return !(worstExcessDb > 0.0) && family.unstableMembers == 0; }
};

SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications,
                                              const ParameterGrids * sweep = nullptr);

}

#endif
