/**
 * @file
 * @brief Tests of algorithm MC3 on QFT toolbox example 2.
 *
 * MC3 keeps the gain out of the search tree: it bisects the zeros and poles
 * only and reads the admissible gains of a whole box off the boundary
 * columns as a union of intervals. It stops on a different measure from
 * MC2, so its answer differs, but it must be an answer to the same problem:
 * at or above 556.913, the optimum of this structure found by brute force
 * over the phase grid, within a fifth of a per cent of MC2's 557.0241, and
 * pinned at 558.397 under the published reading of the columns. Under the
 * conservative reading, where both nodes bracketing a phase must allow the
 * box, the design it returns must pass the direct check against the
 * specifications, of which the boundaries are only a discretisation.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/math/range.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/project/settings.h"

using namespace qftbx;

namespace {

void publishedReading(ProjectController & controller)
{
    Settings published;
    published.algorithms.conservativeBoundaryColumns = false;
    controller.applySettings(published);
}

const auto near = [](double value, double expected) {
    return std::abs(value - expected) <= std::abs(expected) * 1e-4 + 1e-12;
};

}

TEST(Mc3, TheGainStaysOutOfTheTreeOnExampleTwo)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    publishedReading(controller);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc3, Range(1e-9, 10.0), 100));

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(result->numerator().size(), 1u);
    ASSERT_EQ(result->denominator().size(), 1u);

    EXPECT_TRUE(near(result->gain().range().min, 558.39728)) << result->gain().range().min;
    EXPECT_TRUE(near(result->numerator()[0].range().min, 2.0627810)) << result->numerator()[0].range().min;
    EXPECT_TRUE(near(result->denominator()[0].range().min, 138.23722)) << result->denominator()[0].range().min;

    EXPECT_GE(result->gain().range().min, 556.913);
    EXPECT_LT(std::abs(result->gain().range().min / 557.0241 - 1.0), 0.005);
}

TEST(Mc3, WhatItReturnsUnderTheConservativeReadingSatisfiesTheSpecifications)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    Settings conservative;
    conservative.algorithms.conservativeBoundaryColumns = true;
    controller.applySettings(conservative);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc3, Range(1e-9, 10.0), 100));

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(near(result->gain().range().min, 569.90818)) << result->gain().range().min;

    const std::optional<SpecificationCheck> & check = controller.loopShapingResult()->check();
    ASSERT_TRUE(check.has_value());
    EXPECT_TRUE(check->satisfied()) << "worst excess " << check->worstExcessDb << " dB";
}
