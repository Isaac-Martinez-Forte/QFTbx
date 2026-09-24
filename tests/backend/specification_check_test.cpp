/**
 * @file
 * @brief The returned controller checked against the specifications themselves.
 *
 * The boundaries are the specifications mapped onto a phase grid and read off
 * a sampled template, and neither step errs on the safe side, so a point the
 * boundaries accept can violate the specification. Each algorithm is run on
 * QFT toolbox example 2 and on the ACC'90 benchmark, the closed loop is
 * evaluated over the full template at the returned controller, and the worst
 * excess over the bounds is pinned: to 2 mdB where every machine returns the
 * same design, and to hundredths where the search lands on another box of an
 * optimum that many realise. Whether the design satisfies every bound is
 * pinned in all of them. Under the published reading of the columns the excess
 * is recorded as the state of the chain, a positive value being a violation
 * the search did not see; under the conservative reading it is at or below
 * zero. A NaN pin only prints the value. The run attaches the same check to
 * its result; a result read from a file has none.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <string>

#include "src/app/project_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"

using namespace qftbx;

namespace {

struct CheckCase {
    const char * name;
    const char * file;
    LoopShapingAlgorithm algorithm;
    double knownWorstExcessDb;
    bool conservativeColumns = false;
    double toleranceDb = 2e-3;
};

void PrintTo(const CheckCase & c, std::ostream * os)
{
    *os << c.name;
}

class ReturnedControllerAgainstSpecifications : public ::testing::TestWithParam<CheckCase>
{
};

TEST_P(ReturnedControllerAgainstSpecifications, WorstExcessIsPinned)
{
    const CheckCase c = GetParam();

    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR) + "/" + c.file);

    Settings settings;
    settings.algorithms.conservativeBoundaryColumns = c.conservativeColumns;
    controller.applySettings(settings);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, c.algorithm, Range(1e-9, 10.0), 100)) << c.name;

    LtiSystem * result = controller.loopShapingResult()->controller();
    ASSERT_NE(result, nullptr);

    const SpecificationSet specifications = toSpecificationSet(*controller.specifications());

    const SpecificationCheck check = checkAgainstSpecifications(
                *result, *controller.plant(), *controller.omega()->values(),
                controller.templates(), specifications);

    ASSERT_FALSE(check.entries.empty()) << c.name << ": no active specification at any frequency";

    std::printf("SPEC-CHECK %-14s k=%.6f worst excess %+.4f dB\n",
                c.name, result->gain().nominal(), check.worstExcessDb);
    for (const SpecificationExcess & e : check.entries) {
        if (e.excessDb > -0.5) {
            std::printf("           w=%-6g %-18s value %9.4f dB  bound %9.4f dB  excess %+.4f dB\n",
                        e.omega, specificationName(e.type).c_str(), e.valueDb, e.boundDb, e.excessDb);
        }
    }
    std::fflush(stdout);

    EXPECT_TRUE(std::isfinite(check.worstExcessDb)) << c.name;

    if (std::isnan(c.knownWorstExcessDb)) {
        return;
    }

    EXPECT_EQ(check.satisfied(), c.knownWorstExcessDb <= 0.0)
            << c.name << " worst excess " << check.worstExcessDb << " dB";

    EXPECT_NEAR(check.worstExcessDb, c.knownWorstExcessDb, c.toleranceDb) << c.name;
}

constexpr double kUnpinned = std::numeric_limits<double>::quiet_NaN();

INSTANTIATE_TEST_SUITE_P(
    Algorithms, ReturnedControllerAgainstSpecifications,
    ::testing::Values(
        CheckCase{"Ex2NT", "qft_toolbox_ex2.qft", qftbx::nt, +0.0510},
        CheckCase{"Ex2NK", "qft_toolbox_ex2.qft", qftbx::nk, +0.0508},
        CheckCase{"Ex2Mc1", "qft_toolbox_ex2.qft", qftbx::mc1, +0.0508},
        CheckCase{"Ex2McThesis", "qft_toolbox_ex2.qft", qftbx::mc_thesis, +0.0351},
        CheckCase{"Ex2Mc2", "qft_toolbox_ex2.qft", qftbx::mc2, +0.0511, false, 2e-2},
        CheckCase{"Ex2McThesisConservative", "qft_toolbox_ex2.qft", qftbx::mc_thesis, -0.0045, true},
        CheckCase{"Ex2Mc2Conservative", "qft_toolbox_ex2.qft", qftbx::mc2, -0.0001, true},
        CheckCase{"Acc90NT", "acc90.qft", qftbx::nt, kUnpinned},
        CheckCase{"Acc90Mc2", "acc90.qft", qftbx::mc2, kUnpinned}),
    [](const ::testing::TestParamInfo<CheckCase> & info) {
        return std::string(info.param.name);
    });

}

TEST(SpecificationCheckOnTheResult, TheRunAttachesItAndAFileDoesNot)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100));

    LoopShapingResult * result = controller.loopShapingResult();
    ASSERT_NE(result, nullptr);
    ASSERT_TRUE(result->check().has_value()) << "the run checks its controller as its last step";

    const SpecificationCheck direct = checkAgainstSpecifications(
                *result->controller(), *controller.plant(), *controller.omega()->values(),
                controller.templates(), toSpecificationSet(*controller.specifications()));
    EXPECT_DOUBLE_EQ(result->check()->worstExcessDb, direct.worstExcessDb);
    EXPECT_EQ(result->check()->entries.size(), direct.entries.size());

    ProjectController loaded;
    loaded.load(std::string(QFTBX_TEST_DATA_DIR "/planta2.qft"));
    if (loaded.loopShapingResult() != nullptr) {
        EXPECT_FALSE(loaded.loopShapingResult()->check().has_value());
    }
}
