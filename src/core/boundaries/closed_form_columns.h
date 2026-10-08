/**
 * @file
 * @brief The allowed gain intervals of one specification, per phase, in
 * closed form.
 *
 * For each phase the set of gains at which no plant of the template violates
 * the specification is solved exactly from the template contour, instead of
 * being read off a sampled sheet; the plants along each contour segment are
 * covered, and the polygon where the closed loop is singular is forbidden.
 *
 * At a fixed phase of the nominal loop, \f$L = g e^{j\phi}\f$, each
 * magnitude specification is a quadratic inequality in g for each plant of
 * the template (Chait and Yaniv 1993; Moreno, Banos and Berenguel 2006,
 * table I). With \f$q = P_0/P\f$ and \f$c = \mathrm{Re}(\bar{q} e^{j\phi})\f$:
 * - stability, sensor noise, \f$|L/(1+L)| \le W\f$: \f$(W^2 - 1) g^2 + 2 W^2 c g + W^2 |q|^2 \ge 0\f$;
 * - output disturbance, \f$|1/(1+L)| \le W\f$: \f$W^2 g^2 + 2 W^2 c g + (W^2 - 1) |q|^2 \ge 0\f$;
 * - input disturbance, \f$|P/(1+L)| \le W\f$: \f$W^2 g^2 + 2 W^2 c g + W^2 |q|^2 - |P_0|^2 \ge 0\f$;
 * - control effort, \f$|G/(1+L)| \le W\f$: \f$(W^2 - 1/|P|^2) g^2 + 2 W^2 c g + W^2 |q|^2 \ge 0\f$.
 *
 * Each solution set is an interval of g, its complement, everything or
 * nothing, and the column is the intersection over the plants, kept exactly
 * as a RangeUnion: that is where the boundary is multivalued, and only
 * collapsing it to its minimum and maximum makes it look at most
 * bi-valued. Tracking is not here: its inequality is per pair of plants,
 * quadratic in their number, and the sheet wins (Moreno's table II). A
 * plant on the segment between two contour samples may forbid a gain both
 * samples allow, so every segment is a plant too, read as the
 * singular-locus guard reads it: the numerator bounded over its ends, the
 * denominator by the distance from -L to the segment, a linear inequality
 * in g along the ray; and the gains at which -L falls inside the polygon
 * are forbidden. A cloud has no polygon and its guard no clean closed form,
 * so the engine keeps the sheet's columns for a cloud. Roots use the stable
 * form of the quadratic formula, and a zero leading coefficient falls to
 * the linear case. allowedGains() answers in linear gain, g > 0, and
 * columns() in dB over the phase grid; its locus may be null.
 */

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

class ClosedFormColumns
{
public:
    static RangeUnion allowedGains(SpecificationType type, double boundLinear,
                                   std::complex<double> p0, std::complex<double> p,
                                   std::complex<double> nominalOverP, double phaseDegrees);

    static BoundaryColumns columns(SpecificationType type, double boundDb,
                                   std::complex<double> p0, const ComplexCloud & valueSet,
                                   const std::vector<std::complex<double>> & nominalOverP,
                                   const std::vector<double> & phasesDegrees, Range phaseRange,
                                   const SingularLocus * locus);

    static bool covers(SpecificationType type);
};

}

#endif
