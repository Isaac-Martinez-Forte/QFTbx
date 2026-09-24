/**
 * @file
 * @brief Tests of the statistics a loop-shaping run reports about itself.
 *
 * The counters are what the benchmarks and the interface read instead of a
 * printout: elapsed time, peak live nodes, nodes processed, boxes classified
 * with exactly one verdict each, and stability verdicts and profiles. A
 * search that bisects meets ambiguous boxes, one that finds a solution meets
 * feasible ones, and on QFT toolbox example 2 it prunes infeasible ones; MR
 * works on constraints, not boundaries, so it classifies no box. The cases
 * run under the published reading of the boundary columns.
 */

#include <gtest/gtest.h>

#include <string>

#include "src/app/project_controller.h"
#include "src/core/project/settings.h"
#include "src/core/loopshaping/loop_shaping_result.h"

using namespace qftbx;

TEST(LoopShapingStatistics, ARunReportsWhatItCost)
{
    ProjectController controller;
    {
        qftbx::Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
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
    {
        qftbx::Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt, qftbx::Range(1e-9, 10.0), 100));

    const LoopShapingStatistics & statistics = controller.loopShapingResult()->statistics();
    EXPECT_EQ(statistics.boxesFeasible + statistics.boxesInfeasible + statistics.boxesAmbiguous,
              statistics.boxesClassified);
    EXPECT_GT(statistics.boxesAmbiguous, 0u);
    EXPECT_GT(statistics.boxesFeasible, 0u);
    EXPECT_GT(statistics.boxesInfeasible, 0u);
}

TEST(LoopShapingStatistics, MrCountsNoBoxClassification)
{
    ProjectController controller;
    {
        qftbx::Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        controller.applySettings(published);
    }
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mr, qftbx::Range(1e-9, 10.0), 100));

    const LoopShapingStatistics & statistics = controller.loopShapingResult()->statistics();
    EXPECT_GE(statistics.nodesProcessed, 1u);
    EXPECT_EQ(statistics.boxesClassified, 0u) << "MR works on constraints, not on boundaries";
}
