// End-to-end characterisation goldens for the five loop-shaping algorithms
// (phase 8b.0 safety net), run through Controlador on planta1.qft with a
// fixed parameter set. planta1's stored controller has fixed zero/pole and
// an uncertain gain, so the search is one-dimensional and near-instant:
// these goldens pin behaviour, they do not exercise the full search (the
// thesis chapter 6 cases will).
//
// Pinned observations (updated after the 8b.1 interval-extension fix):
// - planta1 only uses the stability specification and its loop magnitude
//   is far below the boundary, so the TRUE optimum is the bottom of the
//   gain range, k = 1. NT and NK now find it (they returned 125.75 while
//   the branch mapping excluded true phases from the projected boxes).
// - MC-prev and MC still return k = 68.75: THEIR phase machinery
//   (feasible/phase cutting) computes phases on its own and is now the
//   suspect. To dissect against the papers in 8b.5/8b.6.
// - MR (rambabu) returns k = [1, 1], the lower end of the search space,
//   consistent with its constraint rules being unreachable (the
//   frecfinal <= 0 masks) and the contraction never firing: the algorithm
//   is known to be unfinished. Indistinguishable from the true optimum
//   here (also 1) - the benchmark goldens expose it. To resolve in 8b.4.

#include <gtest/gtest.h>

#include <QPointF>
#include <QString>

#include "Modelo/controlador.h"

namespace {

struct GoldenResult {
    const char* name;
    tools::alg_loop_shaping algorithm;
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

    Controlador controller;
    delete controller.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    const bool ok = controller.calcularLoopShaping(
        0.5, golden.algorithm, QPointF(1e-9, 10.0), 100,
        false, 10.0, 0, false, false, false, false);

    ASSERT_TRUE(ok) << golden.name;

    LtiSystem* result = controller.getLoopShaping()->getControlador();
    ASSERT_NE(result, nullptr);

    EXPECT_NEAR(result->gain()->range().x(), golden.gain, golden.tolerance) << golden.name;
    EXPECT_NEAR(result->gain()->range().y(), golden.gain, golden.tolerance) << golden.name;

    ASSERT_EQ(result->numerator()->size(), 1);
    ASSERT_EQ(result->denominator()->size(), 1);
    EXPECT_NEAR(result->numerator()->at(0)->range().x(), 42.0, 1e-9) << golden.name;
    EXPECT_NEAR(result->denominator()->at(0)->range().x(), 165.0, 1e-9) << golden.name;
}

INSTANTIATE_TEST_SUITE_P(
    Algorithms, LoopShapingGolden,
    ::testing::Values(
        GoldenResult{"NT", tools::sachin, 1.0, 1e-6},
        GoldenResult{"NK", tools::nandkishor, 1.0, 1e-6},
        // BUG: pinned suspicious result, see the header comment.
        GoldenResult{"MR", tools::rambabu, 1.0, 1e-6},
        // MC-prev and MC show a mild run-to-run wobble (~1e-5 relative):
        // another observation for 8b.5/8b.6.
        GoldenResult{"MCprev", tools::primer_articulo, 68.7508, 1e-3},
        GoldenResult{"MC", tools::segundo_articulo, 68.7508, 1e-3}),
    [](const ::testing::TestParamInfo<GoldenResult>& info) {
        return std::string(info.param.name);
    });

} // namespace
