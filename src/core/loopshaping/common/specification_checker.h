#ifndef QFTBX_SPECIFICATION_CHECKER_H
#define QFTBX_SPECIFICATION_CHECKER_H

#include <cstddef>
#include <limits>
#include <vector>

#include "src/core/specifications/specification.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"

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
 * It exists so that "the returned controller satisfies its specifications"
 * can be a test in the suite instead of a claim.
 */
namespace qftbx {

/// One active specification at one design frequency: the value the
/// controller achieves, the bound, and the difference (positive violates).
struct SpecificationExcess
{
    std::size_t frequencyIndex = 0;
    double omega = 0.0;
    SpecificationType type = SpecificationType::Stability;
    double valueDb = 0.0;
    double boundDb = 0.0;
    double excessDb = 0.0;
};

/// The whole check: every active specification at every design frequency,
/// and the largest excess among them.
struct SpecificationCheck
{
    std::vector<SpecificationExcess> entries;
    double worstExcessDb = -std::numeric_limits<double>::infinity();

    /// No active specification is exceeded.
    bool satisfied() const { return !(worstExcessDb > 0.0); }
};

/**
 * @brief Check a controller against the specifications over a value set.
 *
 * @param controller the controller to verify; evaluated at its nominal values.
 * @param plant the plant; its nominal value at each frequency is \f$ P_0 \f$.
 * @param omega the design frequencies.
 * @param templates the value set of the plant family at each frequency, in
 *        the order of omega. Pass the full clouds: the contour is enough for
 *        the boundaries only under conditions this checker does not assume.
 * @param specifications the seven slots; unused ones and frequencies outside
 *        a band are skipped, and a tracking band needs both T_L and T_U.
 *
 * A frequency whose value set is empty contributes nothing.
 */
SpecificationCheck checkAgainstSpecifications(LtiSystem & controller, LtiSystem & plant,
                                              const std::vector<double> & omega,
                                              const CloudSet & templates,
                                              const SpecificationSet & specifications);

} // namespace qftbx

#endif // QFTBX_SPECIFICATION_CHECKER_H
