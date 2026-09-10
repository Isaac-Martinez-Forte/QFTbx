// The statistics a loop-shaping run reports about itself: what the
// benchmarks and the interface read instead of a printout.
#include <gtest/gtest.h>

#include <string>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/loop_shaping_result.h"

using namespace qftbx;

TEST(LoopShapingStatistics, ARunReportsWhatItCost)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, qftbx::Range(1e-9, 10.0), 100));

    const LoopShapingStatistics & statistics = controller.loopShapingResult()->statistics();
    EXPECT_GT(statistics.milliseconds, 0.0);
    EXPECT_GE(statistics.peakLiveNodes, 1u);
    EXPECT_GE(statistics.nodesProcessed, 1u);
    EXPECT_GE(statistics.boxesClassified, statistics.nodesProcessed) << "every node is classified at every frequency";
    EXPECT_GE(statistics.stabilityVerdicts, 1u) << "the returned point was certified";
    EXPECT_LE(statistics.stabilityProfiles, statistics.stabilityVerdicts);
    EXPECT_EQ(statistics.boxesFeasible + statistics.boxesInfeasible + statistics.boxesAmbiguous,
              statistics.boxesClassified) << "every classified box has exactly one verdict";
}

TEST(LoopShapingStatistics, ABoundaryDrivenSearchSplitsItsVerdicts)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, qftbx::Range(1e-9, 10.0), 100));

    const LoopShapingStatistics & statistics = controller.loopShapingResult()->statistics();
    EXPECT_EQ(statistics.boxesFeasible + statistics.boxesInfeasible + statistics.boxesAmbiguous,
              statistics.boxesClassified);
    //A search that had to bisect met ambiguous boxes, and one that found a
    //solution met a feasible one; on example 2 it also pruned infeasible ones.
    EXPECT_GT(statistics.boxesAmbiguous, 0u);
    EXPECT_GT(statistics.boxesFeasible, 0u);
    EXPECT_GT(statistics.boxesInfeasible, 0u);
}

TEST(LoopShapingStatistics, MrCountsNoBoxClassification)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mr, qftbx::Range(1e-9, 10.0), 100));

    const LoopShapingStatistics & statistics = controller.loopShapingResult()->statistics();
    EXPECT_GE(statistics.nodesProcessed, 1u);
    EXPECT_EQ(statistics.boxesClassified, 0u) << "MR works on constraints, not on boundaries";
}
