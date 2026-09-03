// Chapter 6 of the thesis exercises every acceleration of algorithm MC on
// its own and in combination. The claim underneath those measurements is
// that the accelerations change the SPEED and the path through the search
// tree, NOT the answer: whatever is switched off, the branch & bound must
// still reach the same optimum, because each acceleration only discards
// boxes it has certified cannot hold a better one.
//
// The seven Strategies flags exist for exactly this and, by decision, are
// not exposed in the interface (a user has no reason to disable a proof).
// So the algorithm is driven directly here, with the plant, the controller
// box, the frequencies and the boundaries taken from a loaded project.
//
// acc90 is the fixture: every variant resolves it in under a quarter of a
// second, and its optimum is known from five independent algorithms.
//
// Measured on acc90 at epsilon 0.5 (single machine, informative only - the
// assertions below are on the gain and on the tree size, never on a clock):
//
//   all on                   166 ms   peak  281 live boxes
//   all off                  167 ms   peak 2203
//   no QSInv magnitude       168 ms   peak  281
//   no QSInv phase           164 ms   peak  281
//   no QSFact magnitude      213 ms   peak  330
//   no QSFact phase          154 ms   peak  281
//   no MG                     27 ms   peak  189
//   no tree bisection        171 ms   peak  281
//   no stages                  8 ms   peak   27
//
// Two of them are worth reading carefully. Switching MG or the execution
// stages OFF makes acc90 both faster and cheaper, so this fixture does not
// reproduce the chapter-6 speedups for those two: acc90 has only the
// stability specification and its optimum sits on the floor of the gain
// box, which is precisely the case where a stage that first pursues a
// feasible point, and a bound that keeps the best gain so far, buy nothing
// they do not also cost. The chapter-6 gains for those come from the harder
// designs; the ex2 fixture is the one to measure them on, and it still runs
// for minutes here (deferred with the rest of the performance work). What
// this file does pin, on every fixture it is pointed at, is the invariant:
// none of the seven changes the answer.

#include <gtest/gtest.h>

#include <string>

#include <cmath>

#include "src/core/loopshaping/algorithm_mc_thesis.h"
#include "src/core/project_controller.h"

namespace {

//The optimum of acc90, reached independently by NT, NK, MR, MC1 and MC.
const double kAcc90Gain = 1000.0;

//Termination accuracy: the same the benchmark goldens use.
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

//Runs MC over acc90 with the given strategies. Returns the gain of the
//controller it designed, and the peak of its live list through peakNodes.
double designedGain(const Strategies & strategies, std::size_t * peakNodes = nullptr)
{
    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    AlgorithmMcThesis mc;
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

    //Relative tolerance, as the benchmark goldens use: the corner the search
    //stops on differs between variants, and so do the zero and the pole it
    //reports, but the OBJECTIVE the branch & bound minimises is the gain.
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
    //The point of chapter 6, as something that can be asserted rather than
    //timed: the peak of the live list is deterministic, a wall clock is not.
    std::size_t peakWithAll = 0;
    std::size_t peakWithNone = 0;

    designedGain(Strategies(), &peakWithAll);
    designedGain(everythingOff(), &peakWithNone);

    EXPECT_LT(peakWithAll, peakWithNone)
        << "all on kept " << peakWithAll << " boxes alive at once, bare "
        << peakWithNone;
}

} // namespace
