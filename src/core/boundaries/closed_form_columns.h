#ifndef QFTBX_CLOSED_FORM_COLUMNS_H
#define QFTBX_CLOSED_FORM_COLUMNS_H

#include <complex>
#include <vector>

#include "src/core/boundaries/boundary_columns.h"
#include "src/core/math/range.h"
#include "src/core/math/range_union.h"
#include "src/core/specifications/specification.h"
#include "src/core/templates/cloud_set.h"

namespace qftbx {

class SingularLocus;

/**
 * @brief The allowed magnitude intervals of one specification, per phase
 * column, in closed form: no magnitude grid, no interpolation.
 *
 * At a fixed phase of the nominal loop, L = g e^{j phi}, each of the five
 * magnitude specifications is a quadratic inequality in g for each plant of
 * the template (Chait and Yaniv 1993; Moreno, Banos and Berenguel 2006,
 * table I). With q = P0/P and c = Re(conj(q) e^{j phi}):
 *
 * - stability, sensor noise   |L/(1+L)| <= W:   (W^2 - 1) g^2 + 2 W^2 c g + W^2 |q|^2 >= 0
 * - output disturbance        |1/(1+L)| <= W:   W^2 g^2 + 2 W^2 c g + (W^2 - 1) |q|^2 >= 0
 * - input disturbance         |P/(1+L)| <= W:   W^2 g^2 + 2 W^2 c g + W^2 |q|^2 - |P0|^2 >= 0
 * - control effort            |G/(1+L)| <= W:   (W^2 - 1/|P|^2) g^2 + 2 W^2 c g + W^2 |q|^2 >= 0
 *
 * The solution set of each is an interval of g, or the complement of one,
 * or everything or nothing; the column of the template is the INTERSECTION
 * over its plants, kept exactly as a union of intervals (RangeUnion), which
 * is where the boundary is multivalued. The published objection to this
 * route - "the boundary is, at most, bi-valued" - holds only because the
 * unions were collapsed to their minimum and maximum; intersected exactly
 * they give every component.
 *
 * Things to keep in mind:
 * - Tracking is not here: its inequality is per PAIR of plants, quadratic
 *   in the number of points, and the sheet wins (Moreno's table II).
 * - Between two consecutive samples of a contour the family's border runs
 *   along the segment joining them, and a gain the samples allow may be
 *   forbidden by a plant on that segment: the exact closed form on a sample
 *   shows slivers of allowed gain between neighbouring plants' discs that a
 *   coarse sheet hides by luck (measured on example 2: 0.12 dB wide, at
 *   several frequencies). So with a contour every segment is a plant too,
 *   read as the singular-locus guard reads it: the magnitude's numerator
 *   bounded over the segment's ends, its denominator by the distance from
 *   -L to the segment, which for the point g e^{j phi} moving along a ray
 *   is a linear inequality in g. And the gains at which -L falls inside the
 *   polygon are forbidden outright. With a cloud there is no polygon and
 *   the columns are the sampled ones.
 * - Roots are taken with the stable form of the quadratic formula; a
 *   leading coefficient at zero degenerates to the linear case.
 */
class ClosedFormColumns
{
public:
    /// The allowed gains (linear, g > 0) of one plant at one phase.
    static RangeUnion allowedGains(SpecificationType type, double boundLinear,
                                   std::complex<double> p0, std::complex<double> p,
                                   std::complex<double> nominalOverP, double phaseDegrees);

    /// The columns of one specification over the phase grid, in dB. 'locus'
    /// may be null; when it holds a contour's polygon, the gains at which
    /// -L is inside the template are excluded.
    static BoundaryColumns columns(SpecificationType type, double boundDb,
                                   std::complex<double> p0, const ComplexCloud & valueSet,
                                   const std::vector<std::complex<double>> & nominalOverP,
                                   const std::vector<double> & phasesDegrees, Range phaseRange,
                                   const SingularLocus * locus);

    /// Whether the closed form covers this specification.
    static bool covers(SpecificationType type);
};

} // namespace qftbx

#endif // QFTBX_CLOSED_FORM_COLUMNS_H
