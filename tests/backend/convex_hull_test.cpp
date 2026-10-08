/**
 * @file
 * @brief The convex hull keeps the corners and nothing else.
 *
 * A square with points inside, on an edge and a repeated corner has its four
 * corners as vertices, counter-clockwise; a repeated point is one vertex and
 * no points are none.
 */

#include <gtest/gtest.h>

#include <complex>
#include <cstddef>
#include <vector>

#include "src/core/math/convex_hull.h"

using namespace qftbx;

TEST(ConvexHull, ASquareWithPointsInsideHasFourVertices)
{
    std::vector<std::complex<double>> points = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {0.5, 0.5}, {0.2, 0.7}, {0.5, 0.0}, {1, 0}};
    const std::vector<std::size_t> hull = math::convexHullVertices(points);
    ASSERT_EQ(hull.size(), 4u);
    for (const std::size_t v : hull) {
        EXPECT_TRUE(v <= 3u) << "vertex index " << v;
    }
    for (std::size_t k = 0; k < 4; ++k) {
        const std::complex<double> & o = points[hull[k]];
        const std::complex<double> & a = points[hull[(k + 1) % 4]];
        const std::complex<double> & b = points[hull[(k + 2) % 4]];
        EXPECT_GT((a.real() - o.real()) * (b.imag() - o.imag()) - (a.imag() - o.imag()) * (b.real() - o.real()), 0.0)
            << "counter-clockwise";
    }
    EXPECT_EQ(math::convexHullVertices({{1, 1}, {1, 1}}).size(), 1u);
    EXPECT_EQ(math::convexHullVertices({}).size(), 0u);
}
