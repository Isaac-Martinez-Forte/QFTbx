/**
 * @file
 * @brief Compares the boundaries from a template's contour and its cloud.
 *
 * The raw sweep is the same whichever sample it is fed, since the worst
 * closed-loop case over a template lies on its border; the guard near the
 * singular locus tells the two apart. The contour is a border, so its
 * extremes are exact on the polygon; the cloud has none and is widened by a
 * first-order bound, so the cloud path is the more conservative and never
 * gives a lower optimum. On QFT toolbox example 2 NT must reach 557.0721774
 * from the contour and 585.8737221 from the cloud, and the contour must be
 * the cheaper to compute.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/project/settings.h"
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
    {
        Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    const auto t0 = std::chrono::steady_clock::now();
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

}

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

    EXPECT_GE(cloud.gain, contour.gain);

    EXPECT_LT(contour.boundaryMs, cloud.boundaryMs);
}
