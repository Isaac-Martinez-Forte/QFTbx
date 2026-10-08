/**
 * @file
 * @brief MC2 under the exact point reading, end to end.
 *
 * Each published problem is loaded as the interface loads it and MC2 runs as
 * it does by default, with the exact point reading, the contraction of the
 * gain and without the feasible magnitude cut, at epsilon 0.5. The result is
 * checked as a user would see it: the design meets every specification over
 * the whole template and closes the loop stably with every plant of the
 * sweep, the run records that it read the points exactly, the certificate's
 * lower bounds stay ordered and at or below the returned gain, and the gain
 * is pinned to the design the search returns on the machine the pins were
 * taken on. A compiler that fuses other products may land a few ulps away,
 * hence the relative tolerance; C-XSC rounds the contraction otherwise than
 * kv and reaches another design of the DC motor, so that pin is each
 * library's own. The certificate is kept and finished with the design; it is
 * kept, finished and read too when a gain box too low for any design leaves
 * the search without one, kept but not finished when the run is cancelled,
 * and not kept at all by another algorithm. With the contraction of the
 * gain, the first-order plant with a delay, whose search split one
 * degenerate parameter for ever, ends in a few thousand boxes with a design
 * the verifier accepts; and two problems without a design end with a proof
 * of it. The ACC'90 benchmark, whose Routh table with the nominal plant asks
 * for k < 40.5 where the box starts at k = 1000, and the unstable maglev
 * with a second pole, which without the contraction ends with boxes
 * discarded on the sweep alone.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/math/range.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/project/settings.h"
#include "src/core/system/parameter.h"

using namespace qftbx;

namespace {

#if defined(QFTBX_INTERVAL_CXSC)
constexpr double kDcmT33Gain = 41.102278928422926;
#else
constexpr double kDcmT33Gain = 41.101421995522088;
#endif

struct ExactCase {
    const char * name;
    const char * file;
    double gain;
};

void PrintTo(const ExactCase & c, std::ostream * os)
{
    *os << c.name;
}

class Mc2UnderTheExactReading : public ::testing::TestWithParam<ExactCase>
{
};

TEST_P(Mc2UnderTheExactReading, ReturnsADesignTheVerifierAcceptsWithItsCertificate)
{
    const ExactCase c = GetParam();

    ProjectController controller;
    controller.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / c.file).string());

    Settings settings;
    settings.research.mc2Reading = Settings::Research::PointReading::Exact;
    controller.applySettings(settings);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100)) << c.name;

    const LoopShapingResult * result = controller.loopShapingResult();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->check().has_value()) << c.name;

    const SpecificationCheck & check = *result->check();
    EXPECT_TRUE(check.satisfied()) << c.name << " worst excess " << check.worstExcessDb << " dB";
    EXPECT_TRUE(check.family.checked) << c.name;
    EXPECT_EQ(check.family.unstableMembers, 0u) << c.name;

    EXPECT_EQ(result->run().pointReading, Settings::Research::PointReading::Exact) << c.name;

    const LoopShapingStatistics::Certificate & certificate = result->statistics().certificate;
    EXPECT_TRUE(certificate.kept) << c.name;
    EXPECT_TRUE(certificate.finished) << c.name;
    EXPECT_TRUE(certificate.exactPoints) << c.name;

    const double gain = result->controller()->gain().nominal();
    EXPECT_LE(certificate.lowerBoundStrict, certificate.lowerBound) << c.name;
    EXPECT_LE(certificate.lowerBound, gain) << c.name;

    EXPECT_NEAR(gain, c.gain, std::abs(c.gain) * 1e-6) << c.name;
}

INSTANTIATE_TEST_SUITE_P(
    PublishedProblems, Mc2UnderTheExactReading,
    ::testing::Values(
        ExactCase{"DcmT33", "dcm-T33.qft", kDcmT33Gain},
        ExactCase{"Toolbox2", "toolbox-2.qft", 568.39377382547423},
        ExactCase{"DcmK", "dcm-k.qft", 42527.370794761417}),
    [](const ::testing::TestParamInfo<ExactCase> & info) {
        return std::string(info.param.name);
    });

TEST(Mc2Certificate, ARunWithoutADesignKeepsItsCertificate)
{
    ProjectController controller;
    controller.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / "toolbox-2.qft").string());
    LtiSystem * structure = controller.controllerStructure();
    ASSERT_NE(structure, nullptr);
    controller.setControllerStructure(structure->create("low", structure->numerator(), structure->denominator(),
                                                        Parameter(std::string("k"), Range(1.0, 2.0), 1.0)));

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100), InvalidInput);

    const LoopShapingStatistics statistics = controller.lastLoopShapingStatistics();
    EXPECT_TRUE(statistics.certificate.kept);
    EXPECT_TRUE(statistics.certificate.finished);
    EXPECT_TRUE(statistics.certificate.exactPoints);
    EXPECT_GT(statistics.nodesProcessed, 0u);
    EXPECT_GT(statistics.milliseconds, 0.0);
}

TEST(Mc2Certificate, ACancelledRunKeepsAnUnfinishedCertificateAndOtherAlgorithmsKeepNone)
{
    ProjectController controller;
    controller.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / "toolbox-2.qft").string());
    CancellationToken cancelled;
    cancelled.cancel();

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100, 0, &cancelled), Cancelled);
    EXPECT_TRUE(controller.lastLoopShapingStatistics().certificate.kept);
    EXPECT_FALSE(controller.lastLoopShapingStatistics().certificate.finished);

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::nt, Range(1e-9, 10.0), 100, 0, &cancelled), Cancelled);
    EXPECT_FALSE(controller.lastLoopShapingStatistics().certificate.kept);
    EXPECT_FALSE(controller.lastLoopShapingStatistics().certificate.finished);
}

TEST(Mc2GainContraction, TheDelayedFirstOrderPlantEndsWithADesign)
{
    ProjectController controller;
    controller.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / "fopdt.qft").string());
    Settings settings;
    settings.research.mc2GainContraction = true;
    controller.applySettings(settings);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100));

    const LoopShapingResult * result = controller.loopShapingResult();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->check().has_value());
    EXPECT_TRUE(result->check()->satisfied()) << "worst excess " << result->check()->worstExcessDb << " dB";
    EXPECT_LT(result->statistics().nodesProcessed, 50000u);
    EXPECT_LE(result->statistics().certificate.lowerBound, result->controller()->gain().nominal());
}

TEST(Mc2GainContraction, TheAcc90BenchmarkEndsWithAProofThatNoDesignExists)
{
    ProjectController controller;
    controller.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / "acc90.qft").string());
    ASSERT_GE(controller.controllerStructure()->gain().range().min, 1000.0);
    Settings settings;
    settings.research.mc2GainContraction = true;
    controller.applySettings(settings);

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100), InvalidInput);

    const LoopShapingStatistics & statistics = controller.lastLoopShapingStatistics();
    ASSERT_TRUE(statistics.certificate.finished);
    EXPECT_EQ(statistics.certificate.residueNodes, 0u);
    EXPECT_EQ(statistics.certificate.unprovenDiscards, 0u);
    EXPECT_EQ(statistics.certificate.gridBackedPrunes, 0u) << "a box was discarded without a proof";
    EXPECT_LT(statistics.nodesProcessed, 1000u);
}

TEST(Mc2GainContraction, TheUnstableMaglevWithASecondPoleEndsWithAProofThatNoDesignExists)
{
    for (const bool contraction : {false, true}) {
        ProjectController controller;
        controller.load((std::filesystem::path(QFTBX_EXAMPLES_DIR) / "maglev-upper.qft").string());
        LtiSystem * structure = controller.controllerStructure();
        ASSERT_NE(structure, nullptr);
        std::vector<Parameter> poles = structure->denominator();
        poles.emplace_back(std::string("p2"), Range(0.01, 1000.0), 0.01);
        controller.setControllerStructure(structure->create(structure->name(), structure->numerator(), poles,
                                                            structure->gain(), structure->delay()));
        Settings settings;
        settings.research.mc2GainContraction = contraction;
        controller.applySettings(settings);

        EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100), InvalidInput);

        const LoopShapingStatistics::Certificate & certificate = controller.lastLoopShapingStatistics().certificate;
        ASSERT_TRUE(certificate.finished);
        EXPECT_EQ(certificate.residueNodes, 0u);
        EXPECT_EQ(certificate.unprovenDiscards, 0u);
        if (contraction) {
            EXPECT_EQ(certificate.gridBackedPrunes, 0u) << "a box was discarded without a proof";
            EXPECT_GT(certificate.emptiedByZeroExclusion, 0u);
        } else {
            EXPECT_GT(certificate.gridBackedPrunes, 0u);
        }
    }
}

}
