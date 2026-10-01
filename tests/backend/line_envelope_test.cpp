/**
 * @file
 * @brief The lower and upper envelopes of a set of lines are their minimum
 * and maximum.
 *
 * Fifty random sets of one to forty lines, some with two parallel lines,
 * are walked over a grid of gains: the line each envelope names at a gain
 * must give the plain minimum and maximum there, and both envelopes start
 * at zero.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

#include "src/core/math/line_envelope.h"

using namespace qftbx;

TEST(LineEnvelope, LowerAndUpperMatchThePlainExtremes)
{
    std::mt19937 generator(5);
    std::uniform_real_distribution<double> slope(-4.0, 4.0), intercept(-10.0, 10.0);
    for (int trial = 0; trial < 50; ++trial) {
        std::vector<math::Line> lines;
        const std::size_t count = 1 + trial % 40;
        for (std::size_t k = 0; k < count; ++k) {
            lines.push_back({slope(generator), intercept(generator), k});
        }
        if (trial % 3 == 0 && count > 1) {
            lines[1].slope = lines[0].slope;
        }
        const std::vector<math::EnvelopePiece> lower = math::lowerEnvelope(lines);
        const std::vector<math::EnvelopePiece> upper = math::upperEnvelope(lines);
        ASSERT_FALSE(lower.empty());
        EXPECT_EQ(lower.front().from, 0.0);
        EXPECT_EQ(upper.front().from, 0.0);

        const auto activeLine = [](const std::vector<math::EnvelopePiece> & pieces, double g) {
            std::size_t i = 0;
            while (i + 1 < pieces.size() && pieces[i + 1].from <= g) {
                ++i;
            }
            return pieces[i].index;
        };
        for (int k = 0; k <= 200; ++k) {
            const double g = 0.05 * k;
            double minimum = std::numeric_limits<double>::infinity(), maximum = -minimum;
            for (const math::Line & line : lines) {
                minimum = std::min(minimum, line.slope * g + line.intercept);
                maximum = std::max(maximum, line.slope * g + line.intercept);
            }
            const math::Line & low = lines[activeLine(lower, g)];
            const math::Line & high = lines[activeLine(upper, g)];
            EXPECT_NEAR(low.slope * g + low.intercept, minimum, 1e-9 * (1.0 + std::abs(minimum)));
            EXPECT_NEAR(high.slope * g + high.intercept, maximum, 1e-9 * (1.0 + std::abs(maximum)));
        }
    }
}
