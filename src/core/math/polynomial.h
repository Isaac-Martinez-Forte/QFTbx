#ifndef QFTBX_POLYNOMIAL_H
#define QFTBX_POLYNOMIAL_H

#include <complex>
#include <functional>
#include <optional>
#include <vector>

/**
 * @file
 * @brief Roots of a real polynomial, the polynomial behind a function that
 * can only be evaluated, and whether a polynomial is Hurwitz.
 *
 * Coefficients are highest degree first. polynomialRoots drops leading
 * zeros, takes the roots at the origin out exactly and refines the rest
 * with Aberth's iteration from a circle that bounds them all, with no
 * deflation; an imaginary part below the rounding of the real one is set
 * to zero.
 *
 * polynomialCoefficients recovers the coefficients of a polynomial known
 * only through its values, a free-form denominator for instance, as the
 * discrete Fourier transform of samples on a circle, at unit radius and at
 * four times the largest root: a polynomial gives the same coefficients on
 * both, a function that is not one gives nothing. A coefficient below the
 * rounding of the transform is zero.
 *
 * isHurwitz decides by the Routh table whether every root lies strictly in
 * the left half-plane, without finding them; a zero in the first column is
 * not Hurwitz. polynomialProduct and polynomialSum take an empty operand
 * as 1 and 0. rightHalfPlaneCount and imaginaryAxisFrequencies count a
 * root within 1e-7 of the axis, relative to the largest, as on it; the
 * second leaves the origin out.
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
