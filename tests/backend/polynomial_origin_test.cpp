// A polynomial recovered from its values has roots at the origin where its
// trailing coefficients are zero, and those have to come back as zero and not
// as the rounding they were computed with: a double root at the origin left
// as a pair of roots 1e-7 away lands in the right half-plane as often as not,
// and a plant family with a double integrator - the ACC'90 benchmark - is then
// refused as changing its number of unstable poles from one member to the
// next.

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
