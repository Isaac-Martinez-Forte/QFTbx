/**
 * @file
 * @brief What the program found on the machine, and its answer when the
 * GPU is asked for without a card to use.
 *
 * The status is either usable or carries a reason, never both. A usable
 * card is at least a Pascal (compute capability 6.1, the oldest any build
 * makes code for), with memory, multiprocessors, a ratio between single
 * and double precision, and a driver as recent as the runtime.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "tests/gpu/card.h"

using namespace qftbx;

TEST(GpuStatus, TheBuildInstallsAProviderThatSaysWhetherTheCardCanBeUsed)
{
    ASSERT_NE(gpuProvider(), nullptr);

    const GpuStatus & status = gpuProvider()->status();
    EXPECT_NE(status.usable(), !status.reason().rendered().empty()) << status.reason().rendered();
    EXPECT_GE(status.runtimeVersion, 11080);
}

TEST(GpuStatus, AUsableCardIsDescribed)
{
    QFTBX_REQUIRE_CARD();

    const GpuStatus & status = gpuProvider()->status();
    EXPECT_FALSE(status.name.empty());
    EXPECT_GE(status.computeMajor * 10 + status.computeMinor, 61);
    EXPECT_GT(status.memory, 0u);
    EXPECT_GT(status.multiprocessors, 0);
    EXPECT_GE(status.doublePrecisionRatio, 1);
    EXPECT_GE(status.driverVersion, status.runtimeVersion);
}

TEST(GpuStatus, WithoutAUsableCardTheGpuFailsWithTheReason)
{
    if (gputest::cardUsable()) {
        GTEST_SKIP() << "the card can be used";
    }

    ProjectController controller;
    controller.setPlant(std::make_unique<PolynomialForm>(
            std::string("P"), std::vector<Parameter>{Parameter(1.0)},
            std::vector<Parameter>{Parameter(std::string("a"), Range(1.0, 2.0), 1.5), Parameter(1.0)},
            Parameter(1.0), Parameter(0.0)));
    controller.setOmega(std::make_unique<Omega>(0.1, 10.0, 3, logspace(-1.0, 1.0, 3), Omega::LogSpace));

    ParameterGrids grids;
    grids[std::string("a")] = math::linspace(1.0, 2.0, 3);

    try {
        controller.computeTemplates(std::vector<double>(3, 10.0), grids, Processor::Gpu);
        FAIL() << "the GPU must not be used without a card";
    } catch (const ComputationError & error) {
        EXPECT_EQ(std::string(error.what()), gpuProvider()->status().reason().rendered());
    }
    EXPECT_TRUE(controller.templates().empty());
}
