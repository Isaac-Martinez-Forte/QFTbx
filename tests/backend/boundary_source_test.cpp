// The boundaries computed from the template's contour and from its full
// cloud lead the search to the same controller.
//
// The contour is a cost optimisation: the closed-loop worst case over a
// template is attained on its border, so the boundary sweep may read the
// border alone and run eight times faster. That argument holds when the
// template is simply connected and the critical point lies outside it
// (Terasoft manual, section 4; Moreno, Banos and Berenguel 2006, step 2 of
// algorithm 2.1), neither of which the engine checks yet. On the QFT toolbox
// example 2 the two agree to ten digits because the optimum sits far from
// the singular locus. This test fixes that agreement so that a change to the
// contour, to the sweep or to the critical-point handling shows whether it
// moved one path and not the other.

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/math/range.h"

using namespace qftbx;

namespace {

struct Solution {
    double gain, zero, pole;
    double boundaryMs;
};

Solution solveFrom(bool fromContour)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    const auto t0 = std::chrono::steady_clock::now();
    //The grid the fixture's boundaries were computed on.
    const bool bounds = controller.computeBoundaries(Range(-360.0, 0.0), 361,
                                                     Range(-60.0, 160.0), 441,
                                                     1.0e6, fromContour, false);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    EXPECT_TRUE(bounds);

    EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100));
    LtiSystem * result = controller.loopShapingResult()->controller();
    EXPECT_NE(result, nullptr);

    return {result->gain().nominal(), result->numerator()[0].nominal(),
            result->denominator()[0].nominal(), ms};
}

} // namespace

TEST(BoundarySource, ContourAndCloudAgreeOnExample2)
{
    const Solution contour = solveFrom(true);
    const Solution cloud = solveFrom(false);

    std::printf("BOUNDARY-SOURCE contour: k=%.9f z=%.9f p=%.9f (%.0f ms)\n",
                contour.gain, contour.zero, contour.pole, contour.boundaryMs);
    std::printf("BOUNDARY-SOURCE cloud:   k=%.9f z=%.9f p=%.9f (%.0f ms)\n",
                cloud.gain, cloud.zero, cloud.pole, cloud.boundaryMs);
    std::fflush(stdout);

    const auto same = [](double a, double b) { return std::abs(a - b) <= 1e-9 * std::abs(b); };
    EXPECT_TRUE(same(contour.gain, cloud.gain)) << contour.gain << " vs " << cloud.gain;
    EXPECT_TRUE(same(contour.zero, cloud.zero)) << contour.zero << " vs " << cloud.zero;
    EXPECT_TRUE(same(contour.pole, cloud.pole)) << contour.pole << " vs " << cloud.pole;

    //And the contour is the one that earns its keep on cost.
    EXPECT_LT(contour.boundaryMs, cloud.boundaryMs);
}
