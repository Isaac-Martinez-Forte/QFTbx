/**
 * @file
 * @brief MC2 under every bisection rule of research.mc2.bisection.
 *
 * On two published problems, MC2 with the exact reading and each rule (the
 * wider side of the projection, its area, the widest range and the widest
 * range relative to its size) returns a design the verifier accepts, and no
 * run's certificate contradicts another's gain: every lower bound is at or
 * below every returned gain. The rule is read from its key and refused when
 * it is not one of the four.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/math/range.h"
#include "src/core/project/settings.h"
#include "tests/backend/published_problems.h"

using namespace qftbx;

namespace {

Settings settingsFrom(const std::string & content)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "qftbx-mc2-bisection-test.conf";
    std::ofstream(path) << content;
    Settings settings = readSettings(path.string());
    std::filesystem::remove(path);
    return settings;
}

}

TEST(Mc2Bisection, EveryRuleReturnsADesignNoCertificateContradicts)
{
    using Bisection = Settings::Research::Bisection;
    const std::vector<std::pair<Bisection, const char *>> rules = {
        {Bisection::Widest, "widest"}, {Bisection::Area, "area"}, {Bisection::Width, "width"},
        {Bisection::LogWidth, "log-width"}};

    for (const char * name : {"toolbox-2.qft", "dcm-T33-plant-k.qft"}) {
        const std::string file = qftbx_tests::problem(name);
        if (!std::filesystem::exists(file)) {
            GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
        }
        std::vector<double> gains, bounds;
        for (const auto & [rule, label] : rules) {
            ProjectController controller;
            controller.load(file);
            Settings settings;
            settings.research.mc2Reading = Settings::Research::PointReading::Exact;
            settings.research.mc2Bisection = rule;
            controller.applySettings(settings);

            ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::mc2, Range(1e-9, 10.0), 100)) << name << " " << label;
            const LoopShapingResult * result = controller.loopShapingResult();
            ASSERT_NE(result, nullptr);
            ASSERT_TRUE(result->check().has_value());
            EXPECT_TRUE(result->check()->satisfied()) << name << " " << label;
            EXPECT_EQ(result->check()->family.unstableMembers, 0u) << name << " " << label;
            gains.push_back(result->controller()->gain().nominal());
            bounds.push_back(result->statistics().certificate.lowerBound);
            std::printf("MC2-BISECTION %-20s %-10s gain %.10g, lower bound %.10g, %zu nodes\n", name, label, gains.back(),
                        bounds.back(), result->statistics().nodesProcessed);
        }
        for (const double bound : bounds) {
            for (const double gain : gains) {
                EXPECT_LE(bound, gain) << name;
            }
        }
    }
}

TEST(Mc2Bisection, TheRuleIsReadFromItsKey)
{
    for (const auto & [text, rule] : std::vector<std::pair<const char *, Settings::Research::Bisection>>{
             {"widest", Settings::Research::Bisection::Widest},
             {"area", Settings::Research::Bisection::Area},
             {"width", Settings::Research::Bisection::Width},
             {"log-width", Settings::Research::Bisection::LogWidth}}) {
        const Settings settings = settingsFrom(std::string("[research]\nmc2.bisection = ") + text + "\n");
        EXPECT_EQ(settings.research.mc2Bisection, rule) << text;
    }
    EXPECT_THROW(settingsFrom("[research]\nmc2.bisection = sideways\n"), InvalidInput);
}
