// Characterisation tests over the two thesis chapter-6 benchmark fixtures
// (phase 8b.0 safety net):
//
//   qft_toolbox_ex2.qft - Matlab QFT Toolbox design example 2.
//       P(s) = k a / (s (s + a)), k,a in [1,10]; tracking spec
//       alpha/beta + stability gamma = 1.2; Omega = {0.1,0.5,1,2,15,100}.
//   acc90.qft - ACC'90 benchmark.
//       P(s) = e / (s^2 (s^2 + 2 e)), e in [0.5,2], with the light damping
//       the thesis prescribes for the undamped resonance (+ 0.02 s, an
//       xi = 0.01 at the nominal omega_p = 1: a working choice, to confirm);
//       stability gamma = 1.75; Omega = {0.1,0.98,...,100}.
//
// Both fixtures carry the full pipeline (templates on the thesis-defined
// grids, boundaries on the standard (-360,0)x361 / (-60,60)x121 grid) and
// an n = 3 controller structure C(s) = kc (s + z1) / (s + p1) with a wide
// initial search box. The goldens pin CURRENT behaviour as a regression
// net for the algorithm rewrites; correctness is judged against each
// algorithm's paper, not against these values.
//
// Pinned observations to dissect in later tandas:
// - MC-prev reaches a better optimum than NT on BOTH cases (250000 vs
//   328125 and 2.5e7 vs 3.33e7): same suspicion as on planta1 about NT/NK
//   optimality (live-node ordered-list insertion defect). For 8b.2.
// - MR (rambabu) returns the bottom corner of the search box (1e-9
//   everywhere): its constraint rules are unreachable and the contraction
//   never fires, so everything looks feasible. The algorithm is known to
//   be unfinished. For 8b.4.
// - NK CRASHES on both fixtures: a CXSC arithmetic trap inside
//   sqrtx2y2() called from Natura_Interval_extension::get_box() during
//   init_algorithm() (huge parameter boxes produce an invalid/overflowing
//   interval). planta1's one-dimensional search never reached this path.
//   For 8b.1 (get_box) / 8b.3 (NK). Test disabled until fixed.
// - MC CRASHES on both fixtures: SIGSEGV inside ListaOrdenada::insertar()
//   called from Algorithm_segundo_articulo::recortesFeasible() - the same
//   ordered list whose middle-insert defect is pinned in
//   OrderedList.MiddleInsertBreaksTheOrder. For 8b.1 / 8b.6. Test
//   disabled until fixed.

#include <gtest/gtest.h>

#include <QMap>
#include <QPointF>
#include <QString>
#include <QVector>

#include "Modelo/controlador.h"

namespace {

//---------------------------------------------------------------- fixtures

TEST(ThesisBenchmarkFixture, QftToolboxEx2LoadsWithTheFullPipeline)
{
    Controlador controller;
    delete controller.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    LtiSystem* plant = controller.getPlanta();
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::FreeForm);
    EXPECT_TRUE(plant->gain()->isUncertain());
    EXPECT_EQ(plant->gain()->range(), QPointF(1.0, 10.0));
    ASSERT_EQ(plant->numerator()->size(), 1);
    EXPECT_EQ(plant->numerator()->at(0)->range(), QPointF(1.0, 10.0));

    const QVector<qreal> expectedOmega{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};
    ASSERT_NE(controller.getOmega(), nullptr);
    EXPECT_EQ(*controller.getOmega()->getValores(), expectedOmega);

    QVector<tools::dBND*>* specs = controller.getEspecificaciones();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);
    EXPECT_TRUE(specs->at(0)->utilizado);  // tracking lower (alpha)
    EXPECT_TRUE(specs->at(1)->utilizado);  // tracking upper (beta)
    ASSERT_TRUE(specs->at(2)->utilizado);  // stability
    EXPECT_TRUE(specs->at(2)->constante);
    EXPECT_DOUBLE_EQ(specs->at(2)->altura, 1.2);

    ASSERT_NE(controller.getTemplate(), nullptr);
    EXPECT_EQ(controller.getTemplate()->size(), expectedOmega.size());

    ASSERT_NE(controller.getBound(), nullptr);
    ASSERT_EQ(controller.getBound()->boundaries()->size(), expectedOmega.size());
    for (int f = 0; f < expectedOmega.size(); ++f) {
        EXPECT_EQ(controller.getBound()->boundaries()->at(f)->size(), 2)
            << "frequency " << f << " should carry tracking + stability";
    }

    ASSERT_NE(controller.getControlador(), nullptr);
    EXPECT_EQ(controller.getControlador()->type(),
              LtiSystem::SystemType::ZeroPoleGain);
}

TEST(ThesisBenchmarkFixture, Acc90LoadsWithTheFullPipeline)
{
    Controlador controller;
    delete controller.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    LtiSystem* plant = controller.getPlanta();
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::FreeForm);
    EXPECT_FALSE(plant->gain()->isUncertain());
    ASSERT_EQ(plant->numerator()->size(), 1);
    EXPECT_EQ(plant->numerator()->at(0)->range(), QPointF(0.5, 2.0));

    const QVector<qreal> expectedOmega{0.1, 0.98, 0.99, 1.0, 2.0, 5.0,
                                       7.0, 8.5, 10.0, 15.0, 20.0, 100.0};
    ASSERT_NE(controller.getOmega(), nullptr);
    EXPECT_EQ(*controller.getOmega()->getValores(), expectedOmega);

    QVector<tools::dBND*>* specs = controller.getEspecificaciones();
    ASSERT_NE(specs, nullptr);
    ASSERT_EQ(specs->size(), 7);
    EXPECT_FALSE(specs->at(0)->utilizado);
    ASSERT_TRUE(specs->at(2)->utilizado);  // stability, the only spec
    EXPECT_TRUE(specs->at(2)->constante);
    EXPECT_DOUBLE_EQ(specs->at(2)->altura, 1.75);

    ASSERT_NE(controller.getBound(), nullptr);
    ASSERT_EQ(controller.getBound()->boundaries()->size(), expectedOmega.size());
    for (int f = 0; f < expectedOmega.size(); ++f) {
        EXPECT_EQ(controller.getBound()->boundaries()->at(f)->size(), 1)
            << "frequency " << f << " should carry stability only";
    }

    ASSERT_NE(controller.getControlador(), nullptr);
}

//----------------------------------------------------------------- goldens

struct BenchmarkGolden {
    const char* name;
    const char* file;
    tools::alg_loop_shaping algorithm;
    qreal gain;
    qreal zero;
    qreal pole;
};

//Readable test names in ctest (instead of a raw byte dump).
void PrintTo(const BenchmarkGolden& golden, std::ostream* os)
{
    *os << golden.name;
}

class ThesisBenchmarkGolden : public ::testing::TestWithParam<BenchmarkGolden>
{
};

TEST_P(ThesisBenchmarkGolden, ResultIsPinned)
{
    const BenchmarkGolden golden = GetParam();

    Controlador controller;
    delete controller.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR) + "/" + golden.file);

    const bool ok = controller.calcularLoopShaping(
        0.5, golden.algorithm, QPointF(1e-9, 10.0), 100,
        false, 10.0, 0, false, false, false, false);

    ASSERT_TRUE(ok) << golden.name;

    LtiSystem* result = controller.getLoopShaping()->getControlador();
    ASSERT_NE(result, nullptr);

    //Relative tolerance: the exact optimum wobbles with build flags (FP
    //rounding of the bisection), as the planta1 goldens showed.
    const auto near = [](qreal value, qreal expected) {
        return std::abs(value - expected) <=
               std::abs(expected) * 1e-4 + 1e-12;
    };

    EXPECT_TRUE(near(result->gain()->range().x(), golden.gain))
        << golden.name << " gain " << result->gain()->range().x();
    ASSERT_EQ(result->numerator()->size(), 1);
    ASSERT_EQ(result->denominator()->size(), 1);
    EXPECT_TRUE(near(result->numerator()->at(0)->range().x(), golden.zero))
        << golden.name << " zero " << result->numerator()->at(0)->range().x();
    EXPECT_TRUE(near(result->denominator()->at(0)->range().x(), golden.pole))
        << golden.name << " pole " << result->denominator()->at(0)->range().x();
}

INSTANTIATE_TEST_SUITE_P(
    Algorithms, ThesisBenchmarkGolden,
    ::testing::Values(
        BenchmarkGolden{"Ex2NT", "qft_toolbox_ex2.qft", tools::sachin,
                        328125.0, 5000.0, 5000.0},
        // BUG: bottom corner of the search box, see the header comment.
        BenchmarkGolden{"Ex2MR", "qft_toolbox_ex2.qft", tools::rambabu,
                        1e-9, 1e-9, 1e-9},
        BenchmarkGolden{"Ex2MCprev", "qft_toolbox_ex2.qft", tools::primer_articulo,
                        250000.00342968662, 0.14984963916870972, 0.26408434145678922},
        BenchmarkGolden{"Acc90NT", "acc90.qft", tools::sachin,
                        33332824.70703125, 750.0, 1e-9},
        // BUG: bottom corner of the search box, see the header comment.
        BenchmarkGolden{"Acc90MR", "acc90.qft", tools::rambabu,
                        1e-9, 1e-9, 1e-9},
        BenchmarkGolden{"Acc90MCprev", "acc90.qft", tools::primer_articulo,
                        25000000.000570104, 1e-9, 0.67074960321314647}),
    [](const ::testing::TestParamInfo<BenchmarkGolden>& info) {
        return std::string(info.param.name);
    });

//NK and MC crash on these fixtures (see the header comment): the disabled
//tests document the expectation and become live goldens once fixed.
TEST(ThesisBenchmarkCrash, DISABLED_NkCrashesInGetBoxOnWideBoxes)
{
    Controlador controller;
    delete controller.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    EXPECT_TRUE(controller.calcularLoopShaping(
        0.5, tools::nandkishor, QPointF(1e-9, 10.0), 100,
        false, 10.0, 0, false, false, false, false));
}

TEST(ThesisBenchmarkCrash, DISABLED_McCrashesInOrderedListInsert)
{
    Controlador controller;
    delete controller.cargarSistema(
        QStringLiteral(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    EXPECT_TRUE(controller.calcularLoopShaping(
        0.5, tools::segundo_articulo, QPointF(1e-9, 10.0), 100,
        false, 10.0, 0, false, false, false, false));
}

} // namespace
