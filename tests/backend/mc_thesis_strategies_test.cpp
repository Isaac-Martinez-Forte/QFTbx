/**
 * @file
 * @brief Tests that the accelerations of algorithm MC never change its answer.
 *
 * Chapter 6 of the thesis exercises every acceleration of MC alone and in
 * combination; the claim underneath is that they change the speed and the
 * path through the search tree, not the optimum, since each only discards
 * boxes certified not to hold a better one. The seven strategy flags exist
 * for this and are not exposed in the interface, so the algorithm is driven
 * directly with the inputs of a loaded project, under the published reading
 * of the columns. Every variant must reach the gain 1000 of `acc90.qft`,
 * known from five independent algorithms, to 1e-4 relative; and on QFT
 * toolbox example 2 all accelerations on must keep fewer boxes alive at once
 * than none, the peak of the live list being deterministic as a clock is not.
 */

#include <gtest/gtest.h>

#include <string>

#include <cmath>

#include "src/core/loopshaping/mc_thesis/algorithm_mc_thesis.h"
#include "src/app/project_controller.h"
#include "src/core/project/settings.h"

using namespace qftbx;

namespace {

const double kAcc90Gain = 1000.0;

const double kEpsilon = 0.5;

using Strategies = AlgorithmMcThesis::Strategies;

Strategies everythingOff()
{
    Strategies s;
    s.infeasibleMagnitude = false;
    s.infeasiblePhase = false;
    s.feasibleMagnitude = false;
    s.feasiblePhase = false;
    s.bestGain = false;
    s.treeBisection = false;
    s.stages = false;
    return s;
}

struct Variant {
    const char * name;
    Strategies strategies;
};

void PrintTo(const Variant & variant, std::ostream * os)
{
    *os << variant.name;
}

Variant without(const char * name, void (*disable)(Strategies &))
{
    Variant variant{name, Strategies()};
    disable(variant.strategies);
    return variant;
}

double designedGain(const Strategies & strategies, std::size_t * peakNodes = nullptr,
                    const char * fixture = "acc90.qft")
{
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR) + "/" + fixture);

    AlgorithmMcThesis mc;
    {
        Settings published;
        published.algorithms.conservativeBoundaryColumns = false;
        mc.setSettings(published);
    }
    mc.setStrategies(strategies);
    mc.setProblem(project.plant(), project.controllerStructure(),
                 project.omega()->values(), project.boundaries(), kEpsilon);

    const bool solved = mc.solve();

    if (peakNodes != nullptr) {
        *peakNodes = mc.peakLiveNodes();
    }

    if (!solved) {
        return -1.0;
    }

    return mc.controllerStructure()->gain().range().min;
}

class McThesisStrategies : public ::testing::TestWithParam<Variant>
{
};

TEST_P(McThesisStrategies, EveryCombinationReachesTheSameOptimum)
{
    const double gain = designedGain(GetParam().strategies);

    EXPECT_NEAR(gain, kAcc90Gain, kAcc90Gain * 1e-4)
        << GetParam().name << " designed a gain of " << gain;
}

INSTANTIATE_TEST_SUITE_P(
    Accelerations, McThesisStrategies,
    ::testing::Values(
        Variant{"AllOn", Strategies()},
        Variant{"AllOff", everythingOff()},
        without("NoInfeasibleMagnitude", [](Strategies & s){ s.infeasibleMagnitude = false; }),
        without("NoInfeasiblePhase", [](Strategies & s){ s.infeasiblePhase = false; }),
        without("NoFeasibleMagnitude", [](Strategies & s){ s.feasibleMagnitude = false; }),
        without("NoFeasiblePhase", [](Strategies & s){ s.feasiblePhase = false; }),
        without("NoBestGain", [](Strategies & s){ s.bestGain = false; }),
        without("NoTreeBisection", [](Strategies & s){ s.treeBisection = false; }),
        without("NoStages", [](Strategies & s){ s.stages = false; })),
    ::testing::PrintToStringParamName());

TEST(McThesisStrategies, TheAccelerationsShrinkTheSearchTree)
{
    std::size_t peakWithAll = 0;
    std::size_t peakWithNone = 0;

    designedGain(Strategies(), &peakWithAll, "qft_toolbox_ex2.qft");
    designedGain(everythingOff(), &peakWithNone, "qft_toolbox_ex2.qft");

    EXPECT_LT(peakWithAll, peakWithNone)
        << "all on kept " << peakWithAll << " boxes alive at once, bare "
        << peakWithNone;
}

}
