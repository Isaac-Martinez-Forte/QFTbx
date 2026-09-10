// The guard the boundary sweep applies near the singular locus {-P0/P}.
//
// Every magnitude the sheets are built from is infinite where L0 = -P0/P for
// some plant of the family, and the sweep only sees a finite sample of that
// family. Two things follow and both are covered by SingularLocus: a loop
// value INSIDE the template is singular for some plant the sample may not
// contain (step 2 of Moreno, Banos and Berenguel's algorithm 2.1), and a
// loop value close OUTSIDE may see, between two sample points, a magnitude
// higher than either by at most (h/2)|N|/d^2 - a Lipschitz bound with no
// free parameter, h being the spacing and d the distance. Measured on
// example 2 over 2 400 random cells the bound was never exceeded and cost a
// few per cent of cells next to the locus.
//
// The tests below fix the geometry of the guard on a square and its effect
// on a real fixture: a point inside example 2's template at w = 100 is
// forbidden by every sheet, whichever sample the sweep is fed, and the
// controllers the searches return are the same as before, since their
// optimum sits far from the locus.

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <limits>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/boundaries/boundary_columns.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/singular_locus.h"
#include "src/core/math/range.h"

using namespace qftbx;
using Complex = std::complex<double>;

namespace {

//A closed unit square walked counter-clockwise from the origin, the last
//point repeating the first as the epsilon-hull's walk does, 'n' points a side.
std::vector<Complex> square(int n)
{
    std::vector<Complex> walk;
    const auto edge = [&](Complex from, Complex to) {
        for (int i = 0; i < n; ++i) {
            walk.push_back(from + (to - from) * (static_cast<double>(i) / n));
        }
    };
    edge({0.0, 0.0}, {1.0, 0.0});
    edge({1.0, 0.0}, {1.0, 1.0});
    edge({1.0, 1.0}, {0.0, 1.0});
    edge({0.0, 1.0}, {0.0, 0.0});
    walk.push_back({0.0, 0.0});
    return walk;
}

} // namespace

namespace {

//The sampled worst case of a stability-only sweep at L0 over 'points': the
//largest |L0| / |R + L0|, its smallest, and the nearest sample.
WorstCase sampledAt(const std::vector<Complex> & points, Complex L0)
{
    WorstCase w;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const double d = std::abs(points[i] + L0);
        w.stabilityNoise = std::max(w.stabilityNoise, std::abs(L0) / d);
        w.trackingMin = std::min(w.trackingMin, std::abs(L0) / d);
        if (d < w.nearestSample) { w.nearestSample = d; w.nearestIndex = i; }
    }
    return w;
}

} // namespace

TEST(SingularLocus, AContourIsOneLoopAndInsideIsSingular)
{
    const SingularLocus locus(square(10), true);

    EXPECT_TRUE(locus.isContour());
    EXPECT_EQ(locus.loopCount(), 1u);

    EXPECT_DOUBLE_EQ(locus.borderDistance({0.5, 0.5}), 0.0);
    EXPECT_DOUBLE_EQ(locus.borderDistance({0.01, 0.99}), 0.0);
    EXPECT_NEAR(locus.borderDistance({2.0, 0.5}), 1.0, 1e-12);
    EXPECT_NEAR(locus.borderDistance({-0.3, -0.3}), std::abs(Complex(0.3, 0.3)), 1e-12);

    //Inside: every magnitude infinite, the tracking minimum zero.
    const Complex L0(-0.5, -0.5);   //-L0 = (0.5, 0.5), inside
    const WorstCase g = locus.guard(sampledAt(square(10), L0), L0, {1.0, 0.0});
    EXPECT_TRUE(std::isinf(g.stabilityNoise));
    EXPECT_DOUBLE_EQ(g.trackingMin, 0.0);
    EXPECT_TRUE(std::isinf(g.inputDisturbance));
}

TEST(SingularLocus, OutsideTheExtremesAreExactOverThePolygon)
{
    //The pole at (0.55, -0.2): 0.2 from the bottom side, whose nearest chord
    //point is (0.55, 0) - between two vertices, since they sit at multiples
    //of 0.1. So the exact maximum |L0| / 0.2 exceeds the sampled one,
    //|L0| / sqrt(0.2^2 + 0.05^2), and the minimum is |L0| over the distance
    //to the farthest vertex, (0, 1).
    const SingularLocus locus(square(10), true);
    const Complex L0(-0.55, 0.2);   //-L0 = (0.55, -0.2)
    const WorstCase s = sampledAt(square(10), L0);
    const WorstCase g = locus.guard(s, L0, {1.0, 0.0});

    EXPECT_NEAR(g.stabilityNoise, std::abs(L0) / 0.2, 1e-12);
    EXPECT_GT(g.stabilityNoise, s.stabilityNoise);
    EXPECT_NEAR(g.trackingMin, std::abs(L0) / std::abs(Complex(0.55, 1.2)), 1e-12);
    EXPECT_LE(g.trackingMin, s.trackingMin);
    //|P0| over the same distance for the input disturbance.
    EXPECT_NEAR(g.inputDisturbance, 1.0 / 0.2, 1e-12);
    EXPECT_NEAR(locus.guard(s, L0, {3.0, 0.0}).inputDisturbance, 3.0 / 0.2, 1e-12);

    //Far from the border the exact and the sampled extremes agree to the
    //second order in spacing over distance: at ten spacings the chord passes
    //at most (h/2)^2 / (2 d) closer, a relative 1/800.
    const Complex far(-0.5, 1.0);   //-L0 = (0.5, -1.0), 1.0 from the bottom side
    const WorstCase sf = sampledAt(square(10), far);
    const WorstCase gf = locus.guard(sf, far, {1.0, 0.0});
    EXPECT_NEAR(gf.stabilityNoise / sf.stabilityNoise, 1.0, 2e-3);
    EXPECT_GE(gf.stabilityNoise, sf.stabilityNoise);
}

TEST(SingularLocus, TwoConcatenatedLoopsAreBothSeen)
{
    //Two squares walked one after the other, as the hull concatenates the
    //contours of two components; each closes on its own first point.
    std::vector<Complex> walk = square(8);
    for (const Complex & p : square(8)) {
        walk.push_back(p + Complex(3.0, 0.0));
    }

    const SingularLocus locus(walk, true);
    EXPECT_EQ(locus.loopCount(), 2u);
    EXPECT_DOUBLE_EQ(locus.borderDistance({0.5, 0.5}), 0.0);
    EXPECT_DOUBLE_EQ(locus.borderDistance({3.5, 0.5}), 0.0);
    EXPECT_NEAR(locus.borderDistance({2.0, 0.5}), 1.0, 1e-12);   //between them
}

TEST(SingularLocus, ACloudWidensBySpacingAndIsSingularWithinHalfOfIt)
{
    //An unordered 3x3 grid of unit spacing: every sample one unit from its
    //nearest neighbour. At distance d from the nearest sample a point of the
    //family may be h/2 closer, so the maxima widen by d / (d - 1/2).
    std::vector<Complex> cloud;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            cloud.push_back({static_cast<double>(i), static_cast<double>(j)});
        }
    }
    const SingularLocus locus(cloud, false);

    EXPECT_FALSE(locus.isContour());
    EXPECT_EQ(locus.loopCount(), 0u);
    EXPECT_DOUBLE_EQ(locus.largestSpacing(), 1.0);
    ASSERT_EQ(locus.spacings().size(), 9u);

    const Complex L0(-4.0, -2.0);   //-L0 = (4, 2): 2 from the corner (2, 2)
    const WorstCase s = sampledAt(cloud, L0);
    ASSERT_DOUBLE_EQ(s.nearestSample, 2.0);
    const WorstCase g = locus.guard(s, L0, {1.0, 0.0});
    EXPECT_NEAR(g.stabilityNoise, s.stabilityNoise * (2.0 / 1.5), 1e-12);
    EXPECT_NEAR(g.trackingMin, s.trackingMin / (2.0 / 1.5), 1e-12);

    //Within half a spacing of a sample: singular.
    const Complex near(-1.3, -1.0);   //-L0 = (1.3, 1.0): 0.3 from (1, 1)
    EXPECT_TRUE(std::isinf(locus.guard(sampledAt(cloud, near), near, {1.0, 0.0}).stabilityNoise));
}

TEST(SingularLocus, AnEmptySampleGuardsNothing)
{
    const SingularLocus contour(std::vector<Complex>{}, true);
    const SingularLocus cloud(std::vector<Complex>{}, false);
    WorstCase s;
    s.stabilityNoise = 2.0;
    EXPECT_DOUBLE_EQ(contour.guard(s, {1.0, 0.0}, {1.0, 0.0}).stabilityNoise, 2.0);
    EXPECT_DOUBLE_EQ(cloud.guard(s, {1.0, 0.0}, {1.0, 0.0}).stabilityNoise, 2.0);
}

//On example 2 at w = 100 the singular locus spans magnitudes -40 to 0 dB
//and phases -185 to -180 degrees. A loop value inside the template - the
//negative of an interior plant's P0/P - is forbidden by the stability sheet
//whether the sweep is fed the contour (the inside test) or the cloud (the
//sample itself is singular there), where the contour alone used to read a
//finite worst case off its border points.
TEST(SingularLocus, APointInsideExample2sTemplateIsForbiddenFromEitherSample)
{
    for (const bool fromContour : {true, false}) {
        ProjectController controller;
        controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
        ASSERT_TRUE(controller.computeBoundaries(Range(-360.0, 0.0), 361, Range(-60.0, 160.0), 441,
                                                 1.0e6, fromContour, false));

        const std::vector<double> & omega = *controller.omega()->values();
        const std::size_t last = omega.size() - 1;   //w = 100
        ASSERT_DOUBLE_EQ(omega[last], 100.0);

        //An interior plant of the 25x25 sweep: a = b = 5.5 (the fixture's
        //grid runs 1..10 in 25 steps, so 5.5 is the 13th node of each).
        const Complex s(0.0, 100.0);
        const Complex p0 = controller.plant()->evaluate(100.0);
        const Complex pInterior = (5.5 * 5.5) / (s * (s + 5.5));
        const Complex minusR = -p0 / pInterior;

        double phase = std::arg(minusR) * 180.0 / M_PI;
        if (phase > 0.0) phase -= 360.0;
        const double magnitude = 20.0 * std::log10(std::abs(minusR));

        const BoundaryColumns & columns = controller.boundaries()->columns(last);
        EXPECT_FALSE(columns.allows(columns.columnOf(phase), magnitude))
                << (fromContour ? "contour" : "cloud") << ": a singular loop value at phase "
                << phase << " magnitude " << magnitude << " dB reads as allowed";
    }
}
