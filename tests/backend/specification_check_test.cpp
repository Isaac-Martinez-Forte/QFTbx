// The controller a loop-shaping algorithm returns, checked against the
// SPECIFICATIONS - not against the boundaries it searched.
//
// returned_point_test.cpp pins that the returned point classifies as feasible
// against the boundaries. That is necessary and it is not enough: the
// boundaries are the specifications mapped onto a phase grid and read off a
// sampled template, and neither step errs on the safe side. Reading a point's
// phase at its nearest column node admits, at 1 degree, magnitudes the true
// boundary at that phase forbids; and a sampled family underestimates its
// own worst case. So a point the boundaries accept can violate the
// specification, and on the QFT toolbox example 2 it does.
//
// These tests evaluate the closed-loop magnitudes at the returned
// controller's own loop value over the FULL template, and compare them with
// the bounds. Under the published reading of the columns the excess is
// pinned as the state of the chain - a positive value is a violation the
// search did not see - so that a change to the chain shows what it moves;
// under the conservative reading it is asserted to be at or below zero.
//
// MR is not run on example 2: with the epsilon the other goldens use it does
// not finish in a useful time there, which is also why it has no golden on
// that fixture.
//
// A NaN pin means "not pinned yet": the value is printed and only its
// finiteness is checked, which is how a new row is first observed.

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"

using namespace qftbx;

namespace {

struct CheckCase {
    const char * name;
    const char * file;
    LoopShapingAlgorithm algorithm;
    double knownWorstExcessDb;   //NaN: observe and print, do not pin
    bool conservativeColumns = false;
};

void PrintTo(const CheckCase & c, std::ostream * os)
{
    *os << c.name;
}

class ReturnedControllerAgainstSpecifications : public ::testing::TestWithParam<CheckCase>
{
};

TEST_P(ReturnedControllerAgainstSpecifications, WorstExcessIsPinned)
{
    const CheckCase c = GetParam();

    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR) + "/" + c.file);

    Settings settings;
    settings.algorithms.conservativeBoundaryColumns = c.conservativeColumns;
    controller.applySettings(settings);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, c.algorithm, Range(1e-9, 10.0), 100)) << c.name;

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);

    const SpecificationSet specifications = toSpecificationSet(*controller.specifications());

    const SpecificationCheck check = checkAgainstSpecifications(
                *result, *controller.plant(), *controller.omega()->values(),
                controller.templates(), specifications);

    ASSERT_FALSE(check.entries.empty()) << c.name << ": no active specification at any frequency";

    std::printf("SPEC-CHECK %-14s k=%.6f worst excess %+.4f dB\n",
                c.name, result->gain().nominal(), check.worstExcessDb);
    for (const SpecificationExcess & e : check.entries) {
        if (e.excessDb > -0.5) {   //the near and the violated ones only
            std::printf("           w=%-6g %-18s value %9.4f dB  bound %9.4f dB  excess %+.4f dB\n",
                        e.omega, specificationName(e.type).c_str(), e.valueDb, e.boundDb, e.excessDb);
        }
    }
    std::fflush(stdout);

    EXPECT_TRUE(std::isfinite(check.worstExcessDb)) << c.name;

    if (std::isnan(c.knownWorstExcessDb)) {
        return;   //observed, not pinned yet
    }

    //Under the conservative reading what the search returns satisfies the
    //specifications it was given; under the published reading the excess is
    //recorded, not asserted away.
    if (c.conservativeColumns) {
        EXPECT_TRUE(check.satisfied()) << c.name << " exceeds a bound by " << check.worstExcessDb << " dB";
    }

    //Absolute tolerance in dB: the excess is a small difference of two
    //magnitudes and a relative tolerance on it would be meaningless.
    EXPECT_NEAR(check.worstExcessDb, c.knownWorstExcessDb, 2e-3) << c.name;
}

constexpr double kUnpinned = std::numeric_limits<double>::quiet_NaN();

//The state of the chain under the published reading of the columns (the
//nearest node), recorded as observed on 2026-09-09: on example 2 every
//algorithm exceeds the stability bound at w = 100 - NT by +0.0510 dB, NK and
//MC1 by +0.0508, MC (thesis) and MC2 by +0.0351 - and the three
//boundary-driven searches the tracking spread at w = 15 by +0.0031 dB, since
//the nearest 1-degree node admits what the boundary at the point's own phase
//forbids. Under the conservative reading (both nodes bracketing a phase)
//the same searches return controllers that satisfy every specification;
//the two fast ones are pinned that way (NT, NK and MC1 take minutes on this
//fixture under that reading and gave -0.0000, -0.0002 and -0.0002 dB when
//measured once). ACC'90 is satisfied with margin either way: its optimum
//sits at the top of the gain range.
INSTANTIATE_TEST_SUITE_P(
    Algorithms, ReturnedControllerAgainstSpecifications,
    ::testing::Values(
        CheckCase{"Ex2NT", "qft_toolbox_ex2.qft", qftbx::nt, +0.0510},
        CheckCase{"Ex2NK", "qft_toolbox_ex2.qft", qftbx::nk, +0.0508},
        CheckCase{"Ex2Mc1", "qft_toolbox_ex2.qft", qftbx::mc1, +0.0508},
        CheckCase{"Ex2McThesis", "qft_toolbox_ex2.qft", qftbx::mc_thesis, +0.0351},
        CheckCase{"Ex2Mc2", "qft_toolbox_ex2.qft", qftbx::mc2, +0.0351},
        CheckCase{"Ex2McThesisConservative", "qft_toolbox_ex2.qft", qftbx::mc_thesis, -0.0045, true},
        CheckCase{"Ex2Mc2Conservative", "qft_toolbox_ex2.qft", qftbx::mc2, -0.0032, true},
        CheckCase{"Acc90NT", "acc90.qft", qftbx::nt, -4.8608},
        CheckCase{"Acc90Mc2", "acc90.qft", qftbx::mc2, -4.8608}),
    [](const ::testing::TestParamInfo<CheckCase> & info) {
        return std::string(info.param.name);
    });

} // namespace

//The run attaches the check to its result, and it is the same check the
//function computes on the returned controller; a result read from a file
//carries none.
TEST(SpecificationCheckOnTheResult, TheRunAttachesItAndAFileDoesNot)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100));

    LoopShapingResult * result = controller.loopShapingResult();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->check().has_value()) << "the run checks its controller as its last step";

    const SpecificationCheck direct = checkAgainstSpecifications(
                *result->controller(), *controller.plant(), *controller.omega()->values(),
                controller.templates(), toSpecificationSet(*controller.specifications()));
    EXPECT_DOUBLE_EQ(result->check()->worstExcessDb, direct.worstExcessDb);
    EXPECT_EQ(result->check()->entries.size(), direct.entries.size());

    //A project that ships a loop-shaping result has no check for it.
    ProjectController loaded;
    loaded.load(std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));
    if (loaded.loopShapingResult() != nullptr) {
        EXPECT_FALSE(loaded.loopShapingResult()->check().has_value());
    }
}
