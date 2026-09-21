#ifndef QFTBX_POLYNOMIAL_H
#define QFTBX_POLYNOMIAL_H

#include <complex>
#include <functional>
#include <optional>
#include <vector>

/**
 * @file
 * @brief Roots of a real polynomial, and the polynomial behind a function that
 * can only be evaluated.
 *
 * What the stability criterion needs from a plant is where its poles are, and
 * a plant is given in one of four forms; only two of them name their poles.
 * The polynomial form names its denominator coefficients, and a free-form
 * plant names nothing: its denominator is an expression in s. Both reduce to
 * this file. It is used once per plant, never per node, so the choice here is
 * robustness over speed.
 */
namespace qftbx {
namespace math {

/**
 * @brief The roots of a real polynomial given by its coefficients, highest
 * degree first.
 *
 * Leading zeros are dropped and a constant has no roots. Roots at the origin
 * are taken out exactly before the iteration, which is Aberth's: every root is
 * refined at once from starting points on a circle that bounds them all, and
 * the mutual repulsion term keeps two roots from converging to the same place,
 * so no deflation is needed and the accuracy of the last root is that of the
 * first. Complex roots come in conjugate pairs up to rounding; an imaginary
 * part below the rounding of the real one is set to zero.
 */
std::vector<std::complex<double>> polynomialRoots(const std::vector<double> & coefficients);

/**
 * @brief The coefficients, highest degree first, of a polynomial that is only
 * known through its values, or nothing when the function is not one.
 *
 * The function is sampled on a circle, and the discrete Fourier transform of
 * the samples IS the coefficient vector when the function is a polynomial of
 * degree below the number of samples: the sampling is exact and perfectly
 * conditioned, which a Vandermonde fit on the imaginary axis is not. The
 * degree is read off the last coefficient that is not rounding noise. A
 * function that is not a polynomial of degree up to maxDegree (a delay, a
 * rational function that did not cancel, a transcendental) shows up as power
 * beyond maxDegree or as imaginary parts in what must be real coefficients,
 * and then nothing is returned rather than a polynomial nobody wrote.
 *
 * The circle is walked twice, at unit radius and four times farther than the
 * largest root found: a polynomial gives the same coefficients on both, and
 * an entire function - a delay, a sine, which the first circle alone would
 * pass as its truncated Taylor series - does not. The wider circle's
 * coefficients are the ones returned, since that is where a polynomial with
 * roots far from one keeps its precision. A coefficient below the rounding
 * of the transform - the same threshold the degree is read at - is zero and
 * not the noise it came back as: the noise of a coefficient recovered on a
 * circle of radius r is that of the largest term divided by r^k, and left in
 * place it is a root at the origin that solves a few parts in ten million
 * off it, in the right half-plane as often as not.
 */
std::optional<std::vector<double>> polynomialCoefficients(
        const std::function<std::complex<double>(std::complex<double>)> & value,
        int maxDegree = 24);

/// How many roots lie strictly in the right half-plane. A root within 1e-7 of
/// the imaginary axis, relative to the largest root, counts as ON the axis:
/// that is beyond what the roots are computed to, and a pole that close to
/// the axis is undecidable numerically either way.
int rightHalfPlaneCount(const std::vector<std::complex<double>> & roots);

/// The positive frequencies of the roots on the imaginary axis (the same
/// tolerance), ascending, the origin excluded - a root negligible beside the
/// largest counts as the origin: where a loop passes through infinity with a
/// 180 degree fall, which poles at the origin do not.
std::vector<double> imaginaryAxisFrequencies(const std::vector<std::complex<double>> & roots);

}
}

#endif
