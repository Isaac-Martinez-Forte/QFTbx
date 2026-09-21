/**
 * @file
 * @brief Roots at the origin recovered from values come back exact, not as noise.
 *
 * The denominator of the ACC'90 benchmark family, s^2 (s^2 + 0.02 s + 2 ev),
 * is recovered from its values at every one of 625 members. Its two trailing
 * coefficients must be exactly zero and its double root at the origin must not
 * count as a right half-plane pole: a pair a little off the origin lands in
 * the right half-plane as often as not, and the family would be refused as
 * changing its number of unstable poles from one member to the next.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <functional>
#include <vector>

#include "src/core/math/polynomial.h"

TEST(PolynomialOrigin, ADoubleIntegratorHasNoRightHalfPlanePolesAtAnyMemberOfTheFamily)
{
    for (int i = 0; i < 625; ++i) {
        const double ev = 0.5 + 1.5 * i / 624.0;
        const auto denominator = [ev](std::complex<double> s) {
            return s * s * (s * s + 0.02 * s + 2.0 * ev);
        };

        const std::optional<std::vector<double>> coefficients =
            qftbx::math::polynomialCoefficients(denominator, 8);
        ASSERT_TRUE(coefficients.has_value()) << "ev " << ev;
        ASSERT_EQ(coefficients->size(), 5u) << "ev " << ev;
        EXPECT_EQ(coefficients->at(3), 0.0) << "ev " << ev << ": the s coefficient came back as noise";
        EXPECT_EQ(coefficients->at(4), 0.0) << "ev " << ev << ": the constant came back as noise";

        const std::vector<std::complex<double>> roots = qftbx::math::polynomialRoots(*coefficients);
        EXPECT_EQ(qftbx::math::rightHalfPlaneCount(roots), 0) << "ev " << ev;
    }
}
