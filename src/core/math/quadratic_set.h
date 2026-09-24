#ifndef QFTBX_MATH_QUADRATIC_SET_H
#define QFTBX_MATH_QUADRATIC_SET_H

#include "src/core/math/range_union.h"

/**
 * @file
 * @brief The non-negative half-line where a real quadratic is non-negative.
 *
 * Every specification of the toolbox, read along a ray of the Nichols
 * plane, is a quadratic inequality in the loop magnitude g >= 0 (Chait and
 * Yaniv 1993): a g^2 + b g + c >= 0. Its solution set on the half-line is
 * empty, an interval, the whole half-line, or two pieces, and this is the
 * one place that case analysis is written. The roots come from the stable
 * formula, the larger-magnitude one by the quadratic formula and the other
 * as c over it, so that a nearly linear quadratic (the tracking spread at
 * low frequency has a g^2 coefficient of 1e-6) does not lose its small root
 * to cancellation. A vanishing leading coefficient is the linear case, a
 * negative discriminant leaves the sign of a to decide, and the ends are
 * clipped at zero.
 */
namespace qftbx {
namespace math {

RangeUnion whereNonNegative(double a, double b, double c);

}
}

#endif
