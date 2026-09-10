// The alpha-shape: the epsilon-hull by its definition, edge by edge.
//
// What the walk of Nordin gets from a sequence of steps that may not close,
// the alpha-shape gets from one test per edge - a disc of diameter epsilon
// through both ends holds no other point - so it cannot fail, and it returns
// every component and every hole as a loop. These cases pin the shapes the
// walk is tested on (e_hull_test) plus the ones the walk cannot do: holes,
// and the guarantee of closing at the connecting epsilon.

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <set>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/templates/alpha_shape.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {

ComplexCloud grid(int side, double step, std::complex<double> origin = {0.0, 0.0})
{
    ComplexCloud c;
    for (int i = 0; i < side; ++i)
        for (int j = 0; j < side; ++j)
            c.push_back(origin + std::complex<double>(i * step, j * step));
    return c;
}

std::size_t edgeCount(const AlphaShape & s)
{
    std::size_t n = 0;
    for (const auto & loop : s.loops) n += loop.size() > 1 ? loop.size() : 0;
    return n;
}

} // namespace

TEST(AlphaShape, ARegularGridKeepsExactlyItsBorderAsOneLoop)
{
    //9x9 unit grid, epsilon between the spacing and the diagonal: the
    //disc of diameter 1.2 fits between neighbours along the border only.
    const ComplexCloud g = grid(9, 1.0);
    const AlphaShape s = alphaShape(g, 1.2);

    ASSERT_EQ(s.loops.size(), 1u);
    EXPECT_EQ(s.loops[0].size(), 32u) << "the border of a 9x9 grid has 32 points";
    for (const std::int32_t i : s.loops[0]) {
        const std::complex<double> & z = g[static_cast<std::size_t>(i)];
        EXPECT_TRUE(z.real() == 0.0 || z.real() == 8.0 || z.imag() == 0.0 || z.imag() == 8.0)
            << "interior point " << z << " on the contour";
    }
    //Starts at the rightmost (then topmost) point.
    EXPECT_EQ(g[static_cast<std::size_t>(s.loops[0][0])], std::complex<double>(8.0, 8.0));
}

TEST(AlphaShape, TwoBlocksFartherApartThanEpsilonAreTwoLoopsRightmostFirst)
{
    ComplexCloud c = grid(5, 1.0);
    const ComplexCloud far = grid(5, 1.0, {40.0, 0.0});
    c.insert(c.end(), far.begin(), far.end());

    const AlphaShape s = alphaShape(c, 1.5);
    ASSERT_EQ(s.loops.size(), 2u);
    EXPECT_EQ(s.loops[0].size(), 16u);
    EXPECT_EQ(s.loops[1].size(), 16u);
    EXPECT_GE(c[static_cast<std::size_t>(s.loops[0][0])].real(), 40.0) << "the rightmost block first";
}

TEST(AlphaShape, ARingReturnsItsOuterBorderOnly)
{
    //A 9x9 grid without its 3x3 centre: the outer border of 32, and the
    //hole is not returned, as the walk would not return it either.
    ComplexCloud c;
    for (const std::complex<double> & z : grid(9, 1.0)) {
        if (std::abs(z.real() - 4.0) <= 1.0 && std::abs(z.imag() - 4.0) <= 1.0) continue;
        c.push_back(z);
    }
    const AlphaShape s = alphaShape(c, 1.2);
    ASSERT_EQ(s.loops.size(), 1u);
    EXPECT_EQ(s.loops[0].size(), 32u);
}

TEST(AlphaShape, CollinearPointsAreWalkedOutAndBack)
{
    //Five points on a line: every edge is a spike, so the single loop
    //traverses the four edges twice, 8 steps, like the walk does.
    ComplexCloud c{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
    const AlphaShape s = alphaShape(c, 1.5);
    ASSERT_EQ(s.loops.size(), 1u);
    EXPECT_EQ(s.loops[0].size(), 8u);
}

TEST(AlphaShape, TooSmallAnEpsilonLeavesIsolatedPoints)
{
    ComplexCloud c{{0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}};
    const AlphaShape s = alphaShape(c, 0.1);
    ASSERT_EQ(s.loops.size(), 4u) << "four isolated points, four components";
    for (const auto & loop : s.loops) EXPECT_EQ(loop.size(), 1u);
    EXPECT_EQ(edgeCount(s), 0u);
}

TEST(AlphaShape, TheEngineClosesAtTheConnectingEpsilonAndMarksNothing)
{
    //On example 2 the walk needs up to twice the connecting epsilon to
    //close; the alpha-shape closes at it, at every frequency, and the
    //proposal says so without walking a ladder.
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    TemplateEngine engine;
    engine.setClouds(controller.templates());
    engine.setHullMetric(TemplateEngine::HullMetric::Nichols, 1.0);
    engine.setAlphaShapeContour(true);

    const std::vector<TemplateEngine::EpsilonProposal> proposals = engine.proposeEpsilon();
    ASSERT_EQ(proposals.size(), 6u);
    std::vector<double> epsilon;
    for (const auto & p : proposals) {
        EXPECT_TRUE(p.closes);
        EXPECT_GE(p.epsilon, p.connected);
        EXPECT_LT(p.epsilon, p.connected * 1.011);
        epsilon.push_back(p.epsilon);
    }

    ASSERT_TRUE(engine.computeContours(epsilon));
    for (std::size_t i = 0; i < 6; ++i) {
        const TemplateEngine::ContourReport & r = engine.contourReports()[i];
        EXPECT_FALSE(r.relaxed);
        EXPECT_FALSE(r.truncated);
        EXPECT_FALSE(r.wholeCloud);
        EXPECT_EQ(r.components, 1u) << "frequency " << i;
        EXPECT_GT(engine.contours()[i].size(), 4u);
        EXPECT_LT(engine.contours()[i].size(), engine.clouds()[i].size());
        //Closed by its first point.
        EXPECT_EQ(engine.contours()[i].front(), engine.contours()[i].back());
    }
}

TEST(AlphaShape, TheBoundariesFromTheAlphaShapeMatchTheWalksOnExampleTwo)
{
    //Same contour, same boundaries, same controller: the alpha-shape and
    //the walk, both at the epsilon the fixture carries (10, complex plane),
    //lead NT to the same gain to the precision the search stops at.
    const auto solve = [](bool alpha) {
        ProjectController controller;
        controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
        controller.setAlphaShapeContour(alpha);
        const std::size_t f = controller.omega()->values()->size();
        controller.recomputeContour(std::vector<double>(f, 10.0));
        EXPECT_TRUE(controller.computeBoundaries(Range(-360.0, 0.0), 361, Range(-60.0, 160.0), 441,
                                                 1.0e6, true, false));
        EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100));
        return controller.loopShapingResult()->controller()->gain().nominal();
    };
    const double walk = solve(false);
    const double shape = solve(true);
    std::printf("ALPHA ex2 NT: walk k=%.7f  alpha-shape k=%.7f\n", walk, shape);
    std::fflush(stdout);
    EXPECT_NEAR(shape, walk, 1e-3 * walk);
}
