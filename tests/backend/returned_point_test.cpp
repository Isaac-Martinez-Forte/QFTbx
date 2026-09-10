// The controller a loop-shaping algorithm returns satisfies the boundaries it
// searched against, at every design frequency.
//
// The termination on an epsilon-small ambiguous box returns a corner of the
// box. The corner the anti-blocking rule picks assumes the boundary's allowed
// side is up; where a closed boundary crosses the box that corner falls inside
// the forbidden region. On the QFT toolbox example 2 with a loose epsilon the
// four boundary-driven algorithms returned such a point at one or two
// frequencies before the corner was verified (2026-09-07). These tests pin the
// verification: the returned controller, projected as a degenerate box by the
// same extension the search uses, classifies as feasible everywhere.
//
// The epsilons are loose so the runs stay short; the loop shaping itself is
// exercised in Release in a couple of seconds per case.

#include <gtest/gtest.h>

#include <complex>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/math/range.h"

using namespace qftbx;

namespace {

struct ReturnedPointCase {
    const char * name;
    LoopShapingAlgorithm algorithm;
    double epsilon;
};

void PrintTo(const ReturnedPointCase & c, std::ostream * os)
{
    *os << c.name;
}

class ReturnedPointSatisfiesBoundaries : public ::testing::TestWithParam<ReturnedPointCase>
{
};

TEST_P(ReturnedPointSatisfiesBoundaries, OnQftToolboxExample2)
{
    const ReturnedPointCase c = GetParam();

    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    ASSERT_TRUE(controller.computeLoopShaping(c.epsilon, c.algorithm, Range(1e-9, 10.0), 100)) << c.name;

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);

    PointController point;
    point.gain = result->gain().nominal();
    for (Parameter & z : result->numerator()) {
        point.zeros.push_back(z.nominal());
    }
    for (Parameter & p : result->denominator()) {
        point.poles.push_back(p.nominal());
    }

    NaturalIntervalExtension extension;
    BoundaryViolationDetector detector;
    const std::vector<double> & omega = *controller.omega()->values();

    for (std::size_t i = 0; i < omega.size(); ++i) {
        const std::complex<double> p0 = controller.plant()->evaluate(omega[i]);
        const NicholsBox box = extension.nicholsPoint(point, omega[i], p0);
        const BoxClassification verdict = detector.classifyBox(box, controller.boundaries(), i);

        EXPECT_EQ(verdict.flag(), feasible)
            << c.name << ": the returned controller violates the boundary at w = " << omega[i]
            << " (k = " << point.gain << ")";
    }
}

INSTANTIATE_TEST_SUITE_P(Algorithms, ReturnedPointSatisfiesBoundaries,
    ::testing::Values(
        ReturnedPointCase{"nt_eps10", nt, 10.0},
        ReturnedPointCase{"nk_eps10", nk, 10.0},
        ReturnedPointCase{"mc1_eps20", mc1, 20.0},
        ReturnedPointCase{"mc_thesis_eps10", mc_thesis, 10.0}),
    [](const ::testing::TestParamInfo<ReturnedPointCase> & info) { return std::string(info.param.name); });

} // namespace
