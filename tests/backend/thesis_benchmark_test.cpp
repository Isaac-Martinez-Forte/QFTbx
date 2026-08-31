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
// Pinned observations (updated after 8b.2b: nominal stability check per
// Tharewal sec. 3.3.5, the open-boundary parity fix, the contour tracer
// grid fix, and the tracking model swap - see the fixture generator):
// - The fixtures were regenerated: T_L/T_U assigned per Tharewal 2005
//   Example 3.1 (the QFTbx thesis text swaps their names, which makes the
//   allowed tracking band empty and the boundary impossible), a taller
//   magnitude grid so the low-frequency tracking bound fits, and search
//   boxes sized to keep the honest branch & bound tractable.
// - Ex2 NT is NOT pinned: with real tracking bounds the search takes
//   minutes on this box. Performance work is the core of phase 8b and
//   will revisit it with measurements.
// - Acc90 NT returns the bottom gain corner (k = 1000): the benchmark
//   only has the stability specification and the lightly damped plant
//   keeps low-gain loops stable, so the floor of the gain box is the
//   formal optimum (the box floor keeps the trivially sluggish loops
//   meaningful).
// - MR was rebuilt in 8b.4 as the paper's pure ICSP (quadratic
//   constraints from specs and template representatives, HC4 branch &
//   prune, no boundaries). On ACC'90 it reproduces the same optimum as
//   NT and NK through a third independent route. On ex2 the honest
//   constraint search takes minutes on this box, like NT: not pinned
//   (performance work deferred).
// - MC1 was rebuilt in 8b.5 as the MC of its paper
//   (Martinez-Forte and Cervera, IJRNC 2021): NT/NK branch & bound + QS2
//   (magnitude, phase and feasible-boxes cuts) + the prune variable C.
//   On ACC'90 it reproduces the same optimum as NT, NK and MR through a
//   fourth independent route. On ex2 the honest search takes minutes on
//   this box, like NT/NK/MR: not pinned (performance work deferred). The
//   historical implementation discarded the low-gain feasible strip
//   instead of keeping it as the paper's feasible box z', which is why
//   the old pinned values were far above the true optimum.
// - NK was reviewed against its paper in 8b.3 (Quick Solution rebuilt on
//   the closed-form linear equations, local optimisation reconnected with
//   the 10% rule, stability check wired): the process abort disappeared
//   with the dimensionally broken cutting equations. Its ex2 run still
//   takes minutes on this box (performance work deferred); ACC'90 is
//   pinned below.
// - MC (thesis) was rebuilt in 8b.6 against thesis chapters 4-5 (QSInv,
//   QSFact with the feasible boxes UM/UF, MG, tree bisection, execution
//   stages). The legacy shell threw "initial parameter space not valid"
//   on these fixtures; the rebuilt algorithm reproduces the ACC'90
//   optimum of the other four algorithms (the zero/pole land on a
//   different corner of the optimal-gain shelf; the objective is the
//   gain alone).

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
        0.5, golden.algorithm, QPointF(1e-9, 10.0), 100);

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
        BenchmarkGolden{"Acc90NT", "acc90.qft", tools::nt,
                        1000.0, 250.00749999999999, 750.00250000000005},
        //NK's certified local solution realises the same optimal gain as
        //NT's interval descent (the zero/pole sit at the search centre).
        BenchmarkGolden{"Acc90NK", "acc90.qft", tools::nk,
                        1000.0, 500.005, 500.005},
        BenchmarkGolden{"Acc90MR", "acc90.qft", tools::mr,
                        1000.0, 500.005, 500.005},
        BenchmarkGolden{"Acc90Mc1", "acc90.qft", tools::mc1,
                        1000.0, 250.00749999999999, 750.00250000000005},
        BenchmarkGolden{"Acc90McThesis", "acc90.qft", tools::mc_thesis,
                        1000.0, 250.00749999999999, 0.01}),
    [](const ::testing::TestParamInfo<BenchmarkGolden>& info) {
        return std::string(info.param.name);
    });

} // namespace
