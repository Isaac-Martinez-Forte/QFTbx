/**
 * @file
 * @brief Where a quadratic is non-negative, for every sign case.
 *
 * Four hundred quadratics with random coefficients, one in five of them
 * linear and one in seven without a linear term, are compared with a scan of
 * six hundred gains, leaving out the points where the value is too close to
 * zero to tell. A tiny leading coefficient must not lose the root of the
 * linear part beside it.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <random>

#include "src/core/math/quadratic_set.h"
#include "src/core/math/range_union.h"

using namespace qftbx;

TEST(QuadraticSet, EverySignCaseAgreesWithAScan)
{
    std::mt19937 generator(11);
    std::uniform_real_distribution<double> coefficient(-3.0, 3.0);
    int compared = 0;
    for (int trial = 0; trial < 400; ++trial) {
        const double a = trial % 5 == 0 ? 0.0 : coefficient(generator);
        const double b = trial % 7 == 0 ? 0.0 : coefficient(generator);
        const double c = coefficient(generator);
        const RangeUnion set = math::whereNonNegative(a, b, c);
        for (int k = 0; k <= 600; ++k) {
            const double g = 0.01 * k;
            const double value = a * g * g + b * g + c;
            if (std::abs(value) < 1e-9) {
                continue;
            }
            EXPECT_EQ(set.contains(g), value > 0.0) << "a=" << a << " b=" << b << " c=" << c << " g=" << g;
            ++compared;
        }
    }
    EXPECT_GT(compared, 200000);

    EXPECT_TRUE(math::whereNonNegative(1e-6, 2.0, -1.0).contains(0.5000002));
    EXPECT_FALSE(math::whereNonNegative(1e-6, 2.0, -1.0).contains(0.4999));
}
