#ifndef QFTBX_POLYNOMIAL_H
#define QFTBX_POLYNOMIAL_H

#include <complex>
#include <functional>
#include <optional>
#include <vector>

/**
 * @file
 * @brief Roots of a real polynomial, the polynomial behind a function that can
 * only be evaluated, and whether a polynomial is Hurwitz.
 *
 * What the stability criterion needs from a plant is where its poles are, and
 * a plant is given in one of four forms; only two of them name their poles.
 * The polynomial form names its denominator coefficients, and a free-form
 * plant names nothing: its denominator is an expression in s. Both reduce to
 * this file. Coefficients are always highest degree first.
 *
 * polynomialRoots drops leading zeros, takes the roots at the origin out
 * exactly and refines the rest with Aberth's iteration from a circle that
 * bounds them all: the repulsion term keeps two roots apart, so there is no
 * deflation and the last root is as accurate as the first. An imaginary part
 * below the rounding of the real one is set to zero.
 *
 * polynomialCoefficients recovers the coefficients of a polynomial known only
 * through its values: the discrete Fourier transform of samples on a circle
 * is the coefficient vector, exactly and perfectly conditioned. The circle is
 * walked at unit radius and at four times the largest root: a polynomial gives
 * the same coefficients on both, an entire function (a delay, a sine) does not,
 * and then nothing is returned. The wider circle's coefficients are kept, and
 * one below the rounding of the transform is zero, not the noise it came back
 * as, since that noise would be a root at the origin solved off the axis.
 *
 * isHurwitz decides whether every root lies strictly in the left half-plane
 * without finding them, by the Routh table: every entry of the first column
 * must have the sign of the leading coefficient, and a zero there, a root on
 * the axis, is not Hurwitz. It costs a division per entry, two orders of
 * magnitude less than the roots, which is what makes it affordable for every
 * plant of a family and every candidate a search is about to return.
 *
 * polynomialProduct and polynomialSum take an empty operand as 1 and 0.
 * rightHalfPlaneCount and imaginaryAxisFrequencies count a root within 1e-7
 * of the axis, relative to the largest, as ON it, which is what the roots are
 * computed to; the second leaves the origin out, where a loop does not fall
 * by 180 degrees.
 */
namespace qftbx {
namespace math {

std::vector<std::complex<double>> polynomialRoots(const std::vector<double> & coefficients);

std::optional<std::vector<double>> polynomialCoefficients(
        const std::function<std::complex<double>(std::complex<double>)> & value,
        int maxDegree = 24);

bool isHurwitz(const std::vector<double> & coefficients);

std::vector<double> polynomialProduct(const std::vector<double> & a, const std::vector<double> & b);
std::vector<double> polynomialSum(const std::vector<double> & a, const std::vector<double> & b);

int rightHalfPlaneCount(const std::vector<std::complex<double>> & roots);

std::vector<double> imaginaryAxisFrequencies(const std::vector<std::complex<double>> & roots);

}
}

#endif
