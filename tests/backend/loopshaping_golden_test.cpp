// End-to-end characterisation goldens for the five loop-shaping algorithms
// (phase 8b.0 safety net), run through ProjectController on planta1.qft with a
// fixed parameter set. planta1's stored controller has fixed zero/pole and
// an uncertain gain, so the search is one-dimensional and near-instant:
// these goldens pin behaviour, they do not exercise the full search (the
// thesis chapter 6 cases will).
//
// Pinned observations (updated after the 8b.2b open-boundary parity fix):
// - planta1 carries disturbance-rejection specifications whose open
//   boundaries demand |L0| ABOVE them (8..15 dB at 0.1 rad/s). The
//   historical detection had the open-boundary verdicts swapped, so it
//   accepted exactly the violating loops: the k = 125.75 stored in the
//   fixture sits ~25 dB BELOW its own bounds. With the producer and
//   consumer agreeing, no gain inside the stored controller range
//   satisfies the bounds: the problem is genuinely infeasible and the
//   algorithms now say so.
// - MR, rebuilt as the paper's pure ICSP in 8b.4, now
//   validates the specifications: planta1's stored input-disturbance
//   slot holds an invalid constant (magnitude <= 0, the source of NaN
//   heights), so the validating conversion rejects the problem.

#include <gtest/gtest.h>

#include <QPointF>
#include <QString>

#include "src/core/project_controller.h"
#include "src/core/exception.h"

namespace {

struct GoldenResult {
    const char* name;
    tools::LoopShapingAlgorithm algorithm;
    bool solutionExists;
    qreal gain;
    qreal tolerance;
};

//Readable test names in ctest (instead of a raw byte dump).
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
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    if (!golden.solutionExists) {
        EXPECT_THROW(controller.computeLoopShaping(
                         0.5, golden.algorithm, QPointF(1e-9, 10.0), 100),
                     qftbx::InvalidInput)
            << golden.name;
        return;
    }

    const bool ok = controller.computeLoopShaping(
        0.5, golden.algorithm, QPointF(1e-9, 10.0), 100);

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
        GoldenResult{"NT", tools::nt, false, 0.0, 0.0},
        GoldenResult{"NK", tools::nk, false, 0.0, 0.0},
        GoldenResult{"MR", tools::mr, true, 378.58729554473427, 0.05},
        GoldenResult{"MC1", tools::mc1, false, 0.0, 0.0},
        GoldenResult{"McThesis", tools::mc_thesis, false, 0.0, 0.0}),
    [](const ::testing::TestParamInfo<GoldenResult>& info) {
        return std::string(info.param.name);
    });

} // namespace
