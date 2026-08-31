// End-to-end characterisation goldens for the five loop-shaping algorithms
// (phase 8b.0 safety net), run through Controlador on planta1.qft with a
// fixed parameter set. planta1's stored controller has fixed zero/pole and
// an uncertain gain, so the search is one-dimensional and near-instant:
// these goldens pin behaviour, they do not exercise the full search (the
// thesis chapter 6 cases will).
//
// Pinned observations to dissect in later tandas:
// - NT and NK return k = 125.75 while MC-prev and MC return k = 68.75 for
//   the SAME zero/pole. If 68.75 is feasible, NT/NK should find it too:
//   suspect the live-node ordered-list insertion defect (see
//   OrderedList.MiddleInsertBreaksTheOrder) or a feasibility difference.
//   To investigate in 8b.2/8b.3.
// - MR (rambabu) returns k = [1, 1], the lower end of the search space,
//   consistent with its constraint rules being unreachable (the
//   frecfinal <= 0 masks) and the contraction never firing: the algorithm
//   is known to be unfinished. To resolve in 8b.4.

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
        GoldenResult{"NT", tools::sachin, 125.75, 1e-6},
        GoldenResult{"NK", tools::nandkishor, 125.75, 1e-6},
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
