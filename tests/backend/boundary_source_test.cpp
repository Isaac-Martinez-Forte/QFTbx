// The boundaries computed from the template's contour and from its full
// cloud, and the controller each leads the search to.
//
// The raw sweep over a sample is the same whichever sample it is fed, since
// the closed-loop worst case over a template is attained on its border. The
// guard near the singular locus (SingularLocus) tells the two apart: the
// contour is a border, so the extremes over the family are exact on its
// polygon and differ from the sampled ones only next to the locus; the cloud
// has no border, so its extremes are widened by a first-order bound in the
// local spacing, and it guards more widely. The cloud path is therefore the
// MORE CONSERVATIVE of the two - never a lower optimum - and the contour
// path is the tight one, which is what makes the contour the sample to
// prefer for the boundaries when it closes (the full cloud stands in per
// frequency when it does not). On example 2: contour 557.07 (the raw sweep
// gave 556.94; the optimum lies far from the locus), cloud 585.87.
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

TEST(BoundarySource, TheContourIsTheTightPathAndTheCloudTheConservativeOne)
{
    const Solution contour = solveFrom(true);
    const Solution cloud = solveFrom(false);

    std::printf("BOUNDARY-SOURCE contour: k=%.9f z=%.9f p=%.9f (%.0f ms)\n",
                contour.gain, contour.zero, contour.pole, contour.boundaryMs);
    std::printf("BOUNDARY-SOURCE cloud:   k=%.9f z=%.9f p=%.9f (%.0f ms)\n",
                cloud.gain, cloud.zero, cloud.pole, cloud.boundaryMs);
    std::fflush(stdout);

    const auto near = [](double value, double expected) {
        return std::abs(value - expected) <= std::abs(expected) * 1e-4;
    };
    EXPECT_TRUE(near(contour.gain, 557.0721774)) << "contour path gain " << contour.gain;
    EXPECT_TRUE(near(cloud.gain, 585.8737221)) << "cloud path gain " << cloud.gain;

    //The cloud never certifies what the contour forbids near the locus, so
    //its optimum is never the lower one.
    EXPECT_GE(cloud.gain, contour.gain);

    //And the contour is the one that earns its keep on cost.
    EXPECT_LT(contour.boundaryMs, cloud.boundaryMs);
}
