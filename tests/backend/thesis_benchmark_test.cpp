/**
 * @file
 * @brief Goldens of every loop-shaping algorithm on the benchmark fixtures.
 *
 * `qft_toolbox_ex2.qft` is design example 2 of the MATLAB QFT Toolbox, P(s) =
 * k a / (s (s + a)) with k, a in [1, 10], tracking bounds and a stability
 * margin of 1.2; `acc90.qft` is the ACC'90 benchmark, P(s) = e / (s^2 (s^2 +
 * 0.02 s + 2 e)) with e in [0.5, 2] and stability 1.75 as its only
 * specification. The gain, zero and pole of the first-order controller each
 * algorithm returns are pinned to a relative 1e-4, the two best-gain searches
 * also under the conservative reading of the columns. Where the optimum is
 * realised by many zero-pole boxes, which one a search meets follows from the
 * arithmetic of the machine it runs on, so those cases pin the gain within
 * its own tolerance and ask of the zero and the pole only that they lie in
 * the box searched. The values are a regression net; correctness is judged
 * against each algorithm's paper.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <optional>
#include <string>

#include <vector>

#include "src/core/math/point.h"
#include "src/core/math/range.h"

#include "src/app/project_controller.h"
#include "src/core/project/settings.h"

using namespace qftbx;

namespace {

TEST(ThesisBenchmarkFixture, QftToolboxEx2LoadsWithTheFullPipeline)
{
    ProjectController controller;
    controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));

    LtiSystem* plant = controller.plant();
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::FreeForm);
    EXPECT_TRUE(plant->gain().isUncertain());
    EXPECT_EQ(plant->gain().range(), Range(1.0, 10.0));
    ASSERT_EQ(plant->numerator().size(), 1);
    EXPECT_EQ(plant->numerator()[0].range(), Range(1.0, 10.0));

    const std::vector<double> expectedOmega{0.1, 0.5, 1.0, 2.0, 15.0, 100.0};
    ASSERT_NE(controller.omega(), nullptr);
    EXPECT_EQ(*controller.omega()->values(), expectedOmega);

    const qftbx::SpecificationRecords * specs = controller.specifications();
    ASSERT_NE(specs, nullptr);
    EXPECT_TRUE(specs->at(0).used);
    EXPECT_TRUE(specs->at(1).used);
    ASSERT_TRUE(specs->at(2).used);
    EXPECT_TRUE(specs->at(2).constant);
    EXPECT_DOUBLE_EQ(specs->at(2).height, 1.2);

    ASSERT_FALSE(controller.templates().empty());
    EXPECT_EQ(static_cast<int>(controller.templates().size()), expectedOmega.size());

    ASSERT_NE(controller.boundaries(), nullptr);
    ASSERT_EQ(static_cast<int>(controller.boundaries()->boundaries().size()), expectedOmega.size());
    for (int f = 0; f < expectedOmega.size(); ++f) {
        EXPECT_EQ(controller.boundaries()->boundaries().at(static_cast<std::size_t>(f)).size(), 2u)
            << "frequency " << f << " should carry tracking + stability";
    }

    ASSERT_NE(controller.controllerStructure(), nullptr);
    EXPECT_EQ(controller.controllerStructure()->type(),
              LtiSystem::SystemType::ZeroPoleGain);
}

TEST(ThesisBenchmarkFixture, Acc90LoadsWithTheFullPipeline)
{
    ProjectController controller;
    controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    LtiSystem* plant = controller.plant();
    ASSERT_NE(plant, nullptr);
    EXPECT_EQ(plant->type(), LtiSystem::SystemType::FreeForm);
    EXPECT_FALSE(plant->gain().isUncertain());
    ASSERT_EQ(plant->numerator().size(), 1);
    EXPECT_EQ(plant->numerator()[0].range(), Range(0.5, 2.0));

    const std::vector<double> expectedOmega{0.1, 0.98, 0.99, 1.0, 2.0, 5.0,
                                       7.0, 8.5, 10.0, 15.0, 20.0, 100.0};
    ASSERT_NE(controller.omega(), nullptr);
    EXPECT_EQ(*controller.omega()->values(), expectedOmega);

    const qftbx::SpecificationRecords * specs = controller.specifications();
    ASSERT_NE(specs, nullptr);
    EXPECT_FALSE(specs->at(0).used);
    ASSERT_TRUE(specs->at(2).used);
    EXPECT_TRUE(specs->at(2).constant);
    EXPECT_DOUBLE_EQ(specs->at(2).height, 1.75);

    ASSERT_NE(controller.boundaries(), nullptr);
    ASSERT_EQ(static_cast<int>(controller.boundaries()->boundaries().size()), expectedOmega.size());
    for (int f = 0; f < expectedOmega.size(); ++f) {
        EXPECT_EQ(controller.boundaries()->boundaries().at(static_cast<std::size_t>(f)).size(), 1u)
            << "frequency " << f << " should carry stability only";
    }

    ASSERT_NE(controller.controllerStructure(), nullptr);
}

struct BenchmarkGolden {
    const char* name;
    const char* file;
    qftbx::LoopShapingAlgorithm algorithm;
    double gain;
    std::optional<double> zero;
    std::optional<double> pole;
    bool conservativeColumns = false;
    double gainTolerance = 1e-4;
};

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

    ProjectController controller;
    controller.load(
        std::string(QFTBX_TEST_DATA_DIR) + "/" + golden.file);

    qftbx::Settings settings;
    settings.algorithms.conservativeBoundaryColumns = golden.conservativeColumns;
    controller.applySettings(settings);

    LtiSystem* structure = controller.controllerStructure();
    ASSERT_NE(structure, nullptr);
    ASSERT_EQ(structure->numerator().size(), 1);
    ASSERT_EQ(structure->denominator().size(), 1);
    const qftbx::Range zeroBox = structure->numerator()[0].range();
    const qftbx::Range poleBox = structure->denominator()[0].range();

    const bool ok = controller.computeLoopShaping(
        0.5, golden.algorithm, qftbx::Range(1e-9, 10.0), 100);

    ASSERT_TRUE(ok) << golden.name;

    LtiSystem* result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);

    const auto near = [](double value, double expected, double tolerance) {
        return std::abs(value - expected) <=
               std::abs(expected) * tolerance + 1e-12;
    };

    EXPECT_TRUE(near(result->gain().range().min, golden.gain, golden.gainTolerance))
        << golden.name << " gain " << result->gain().range().min;
    ASSERT_EQ(result->numerator().size(), 1);
    ASSERT_EQ(result->denominator().size(), 1);

    const double zero = result->numerator()[0].range().min;
    const double pole = result->denominator()[0].range().min;

    if (golden.zero.has_value()) {
        EXPECT_TRUE(near(zero, *golden.zero, 1e-4)) << golden.name << " zero " << zero;
        EXPECT_TRUE(near(pole, *golden.pole, 1e-4)) << golden.name << " pole " << pole;
    } else {
        EXPECT_GE(zero, zeroBox.min) << golden.name << " zero " << zero;
        EXPECT_LE(zero, zeroBox.max) << golden.name << " zero " << zero;
        EXPECT_GE(pole, poleBox.min) << golden.name << " pole " << pole;
        EXPECT_LE(pole, poleBox.max) << golden.name << " pole " << pole;
    }
}

TEST(ThesisBenchmarkFixture, Acc90MrWithTheNicholsEpsilon)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/acc90.qft"));

    qftbx::Settings settings;
    settings.algorithms.mrNicholsEpsilon = true;
    controller.applySettings(settings);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mr, qftbx::Range(1e-9, 10.0), 100));

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);
    EXPECT_NEAR(result->gain().range().min, 1000.0, 1e-4);
    ASSERT_EQ(result->numerator().size(), 1);
    ASSERT_EQ(result->denominator().size(), 1);
    EXPECT_NEAR(result->numerator()[0].range().min, 500.005, 1e-3);
    EXPECT_NEAR(result->denominator()[0].range().min, 500.005, 1e-3);
}

INSTANTIATE_TEST_SUITE_P(
    Algorithms, ThesisBenchmarkGolden,
    ::testing::Values(
        BenchmarkGolden{"Acc90NT", "acc90.qft", qftbx::nt,
                        1000.0, 500.005, 0.01},
        BenchmarkGolden{"Acc90NK", "acc90.qft", qftbx::nk,
                        1000.0, 500.005, 500.005},
        BenchmarkGolden{"Acc90MR", "acc90.qft", qftbx::mr,
                        1000.0, 500.005, 500.005},
        BenchmarkGolden{"Acc90Mc1", "acc90.qft", qftbx::mc1,
                        1000.0, 500.005, 0.01},
        BenchmarkGolden{"Acc90McThesis", "acc90.qft", qftbx::mc_thesis,
                        1000.0, 500.005, 0.01},
        BenchmarkGolden{"Acc90Mc2", "acc90.qft", qftbx::mc2,
                        1000.0, std::nullopt, std::nullopt},
        BenchmarkGolden{"Ex2NT", "qft_toolbox_ex2.qft", qftbx::nt,
                        556.9433291, 1.87155365, 137.642901},
        BenchmarkGolden{"Ex2NK", "qft_toolbox_ex2.qft", qftbx::nk,
                        556.9619629, 1.869239822, 137.642901},
        BenchmarkGolden{"Ex2Mc1", "qft_toolbox_ex2.qft", qftbx::mc1,
                        556.9603483, 1.868936823, 137.642901},
        BenchmarkGolden{"Ex2McThesis", "qft_toolbox_ex2.qft", qftbx::mc_thesis,
                        567.6912501, 3.305865479, 142.5866992},
        BenchmarkGolden{"Ex2Mc2", "qft_toolbox_ex2.qft", qftbx::mc2,
                        557.0240549, std::nullopt, std::nullopt, false, 1e-2},
        BenchmarkGolden{"Ex2McThesisConservative", "qft_toolbox_ex2.qft", qftbx::mc_thesis,
                        579.4719402, 3.183796387, 144.5398047, true},
        BenchmarkGolden{"Ex2Mc2Conservative", "qft_toolbox_ex2.qft", qftbx::mc2,
                        567.3175312, 1.819430571, 140.0436795, true}),
    [](const ::testing::TestParamInfo<BenchmarkGolden>& info) {
        return std::string(info.param.name);
    });

}
