/**
 * @file
 * @brief MC2 under the exact point reading, end to end.
 *
 * Each published problem is loaded as the interface loads it and MC2 runs
 * with the exact point reading and without the feasible magnitude cut, the
 * configuration the battery chose, at epsilon 0.5. The result is checked as
 * a user would see it: the design meets every specification over the whole
 * template and closes the loop stably with every plant of the sweep, the run
 * records that it read the points exactly, the certificate's lower bounds
 * stay ordered and at or below the returned gain, and the gain is pinned to
 * the design the search returns on the machine the pins were taken on. A
 * compiler that fuses other products may land a few ulps away, hence the
 * relative tolerance.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <ostream>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/math/range.h"
#include "src/core/project/settings.h"

using namespace qftbx;

namespace {

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
    settings.research.mc.feasibleMagnitude = false;
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
    EXPECT_TRUE(certificate.exactPoints) << c.name;

    const double gain = result->controller()->gain().nominal();
    EXPECT_LE(certificate.lowerBoundStrict, certificate.lowerBound) << c.name;
    EXPECT_LE(certificate.lowerBound, gain) << c.name;

    EXPECT_NEAR(gain, c.gain, std::abs(c.gain) * 1e-6) << c.name;
}

INSTANTIATE_TEST_SUITE_P(
    PublishedProblems, Mc2UnderTheExactReading,
    ::testing::Values(
        ExactCase{"DcmT33", "dcm-T33.qft", 41.102278928422926},
        ExactCase{"Toolbox2", "toolbox-2.qft", 568.77281430503876},
        ExactCase{"DcmK", "dcm-k.qft", 42523.25845719401}),
    [](const ::testing::TestParamInfo<ExactCase> & info) {
        return std::string(info.param.name);
    });

}
