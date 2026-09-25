#ifndef QFTBX_MATH_INTERVAL_POLYNOMIAL_H
#define QFTBX_MATH_INTERVAL_POLYNOMIAL_H

#include <vector>

#include "src/core/math/interval.h"

/**
 * @file
 * @brief Polynomials with interval coefficients, and the Routh table that
 * proves every polynomial of such a family non-Hurwitz.
 *
 * A box of controllers closed with one plant of the family gives a
 * characteristic polynomial whose coefficients range over intervals; the
 * intervals computed by interval arithmetic enclose those ranges, so the
 * interval polynomial is a superset of the family of real polynomials the
 * box produces. The Routh table evaluated in interval arithmetic then
 * encloses the entries of every member: when the leading coefficient has a
 * definite sign and some entry of the first column lies entirely on the
 * other side of zero, every member fails Routh's criterion and no
 * controller of the box stabilises that plant. That is a proof, valid for
 * the whole box, in the direction a branch and bound may discard: a box
 * proven unstable holds no design. An entry whose interval contains zero
 * ends the table undecided, since the next row divides by it, and
 * undecided is not unstable. Coefficients are highest degree first, as in
 * polynomial.h; product and sum take an empty operand as 1 and 0.
 */
namespace qftbx {
namespace math {

std::vector<Interval> intervalPolynomialProduct(const std::vector<Interval> & a, const std::vector<Interval> & b);

std::vector<Interval> intervalPolynomialSum(const std::vector<Interval> & a, const std::vector<Interval> & b);

bool provablyNotHurwitz(const std::vector<Interval> & coefficients);

}
}

#endif
