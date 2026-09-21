/**
 * @file
 * @brief Golden tests of the five loop-shaping algorithms on `planta1.qft`.
 *
 * The stored controller has a fixed zero and pole and an uncertain gain, so
 * the search is one-dimensional and near-instant: the goldens pin behaviour
 * rather than exercise a full search. The fixture's disturbance-rejection
 * specifications demand a loop above their open boundaries and no gain in
 * the stored range satisfies them, so NT, NK, MC1 and MC of the thesis must
 * report that no solution exists. MR works from the specifications rather
 * than the boundaries and is pinned at a gain of 378.157; its stop is the
 * width of the controller parameter box (Kalla and Nataraj 2010, section 5)
 * where the other four measure the Nichols box, so the accuracy of 0.5
 * means different things per algorithm.
 */

#include <gtest/gtest.h>

#include <string>

#include "src/core/math/range.h"

#include "src/core/math/point.h"

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"

using namespace qftbx;

namespace {

struct GoldenResult {
    const char* name;
    qftbx::LoopShapingAlgorithm algorithm;
    bool solutionExists;
    double gain;
    double tolerance;
};

void PrintTo(const GoldenResult& golden, std::ostream* os)
{
    *os << golden.name;
}

class LoopShapingGolden : public ::testing::TestWithParam<GoldenResult>
{
};

TEST_P(LoopShapingGolden, Planta1ResultIsPinned)
{
    const GoldenResult golden = GetParam();

    ProjectController controller;
    controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    if (!golden.solutionExists) {
        EXPECT_THROW(controller.computeLoopShaping(
                         0.5, golden.algorithm, qftbx::Range(1e-9, 10.0), 100),
                     qftbx::InvalidInput)
            << golden.name;
        return;
    }

    const bool ok = controller.computeLoopShaping(
        0.5, golden.algorithm, qftbx::Range(1e-9, 10.0), 100);

    ASSERT_TRUE(ok) << golden.name;

    LtiSystem* result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);

    EXPECT_NEAR(result->gain().range().min, golden.gain, golden.tolerance) << golden.name;
    EXPECT_NEAR(result->gain().range().max, golden.gain, golden.tolerance) << golden.name;

    ASSERT_EQ(result->numerator().size(), 1);
    ASSERT_EQ(result->denominator().size(), 1);
    EXPECT_NEAR(result->numerator()[0].range().min, 42.0, 1e-9) << golden.name;
    EXPECT_NEAR(result->denominator()[0].range().min, 165.0, 1e-9) << golden.name;
}

INSTANTIATE_TEST_SUITE_P(
    Algorithms, LoopShapingGolden,
    ::testing::Values(
        GoldenResult{"NT", qftbx::nt, false, 0.0, 0.0},
        GoldenResult{"NK", qftbx::nk, false, 0.0, 0.0},
        GoldenResult{"MR", qftbx::mr, true, 378.15738155492267, 0.05},
        GoldenResult{"MC1", qftbx::mc1, false, 0.0, 0.0},
        GoldenResult{"McThesis", qftbx::mc_thesis, false, 0.0, 0.0}),
    [](const ::testing::TestParamInfo<GoldenResult>& info) {
        return std::string(info.param.name);
    });

}
