/**
 * @file
 * @brief The exact point check is the verifier's specification criterion.
 *
 * On every published problem, thousands of controllers drawn at random from
 * the search box give, through the check, the entries the verifier records
 * on the same controller built as a system: the same values, bounds and
 * excesses to the last bit, and the same verdict up to the tolerance; a
 * structure with a delay is evaluated as the verifier evaluates it. Where
 * the gain is bisected onto the edge of a bound, the verifier accepts the
 * last point on the allowed side and the check does not: its tolerance is
 * what keeps a candidate put on the edge by a root formula from being
 * returned and then refused. A project whose templates do not cover its
 * design frequencies leaves the check unusable.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/specifications/specification_record.h"

using namespace qftbx;

namespace {

std::vector<std::string> publishedProblems()
{
    std::vector<std::string> files;
    if (!std::filesystem::exists(QFTBX_EXAMPLES_DIR)) {
        return files;
    }
    for (const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator(QFTBX_EXAMPLES_DIR)) {
        if (entry.path().extension() == ".qft") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

PointController randomPoint(LtiSystem & box, std::mt19937 & generator)
{
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const auto draw = [&](const Parameter & parameter) {
        if (!parameter.isUncertain()) {
            return parameter.nominal();
        }
        const Range range = parameter.range();
        if (range.min > 0.0) {
            return std::exp(std::log(range.min) + unit(generator) * (std::log(range.max) - std::log(range.min)));
        }
        return range.min + unit(generator) * (range.max - range.min);
    };

    PointController point;
    for (const Parameter & parameter : box.numerator()) point.zeros.push_back(draw(parameter));
    for (const Parameter & parameter : box.denominator()) point.poles.push_back(draw(parameter));
    point.gain = draw(box.gain());
    return point;
}

bool sameBits(double a, double b)
{
    return a == b || (std::isnan(a) && std::isnan(b));
}

void expectTheVerifiersEntries(ExactPointCheck & check, LtiSystem * structure, ProjectController & project,
                               const SpecificationSet & specifications, const std::string & name, int points)
{
    std::mt19937 generator(7);
    std::size_t admitted = 0;

    for (int n = 0; n < points; ++n) {
        const PointController point = randomPoint(*structure, generator);
        std::unique_ptr<LtiSystem> system = systemFromPoint(structure, point);
        const SpecificationCheck verifier = checkAgainstSpecifications(
                    *system, *project.plant(), *project.omega()->values(), project.templates(), specifications);
        const SpecificationCheck exact = check.checkOf(point);

        ASSERT_EQ(exact.entries.size(), verifier.entries.size()) << name << " point " << n;
        for (std::size_t e = 0; e < exact.entries.size(); ++e) {
            EXPECT_EQ(exact.entries[e].frequencyIndex, verifier.entries[e].frequencyIndex) << name << " point " << n;
            EXPECT_EQ(exact.entries[e].type, verifier.entries[e].type) << name << " point " << n;
            EXPECT_TRUE(sameBits(exact.entries[e].valueDb, verifier.entries[e].valueDb))
                << name << " point " << n << " entry " << e << ": " << exact.entries[e].valueDb << " vs " << verifier.entries[e].valueDb;
            EXPECT_TRUE(sameBits(exact.entries[e].boundDb, verifier.entries[e].boundDb)) << name << " point " << n;
            EXPECT_TRUE(sameBits(exact.entries[e].excessDb, verifier.entries[e].excessDb)) << name << " point " << n;
        }
        EXPECT_TRUE(sameBits(exact.worstExcessDb, verifier.worstExcessDb)) << name << " point " << n;

        const bool admits = check.admits(point);
        EXPECT_EQ(admits, verifier.worstExcessDb <= -ExactPointCheck::kToleranceDb) << name << " point " << n;
        admitted += admits ? 1 : 0;
    }

    std::printf("EXACT-POINT %-16s %d points, %zu admitted, %zu kernel passes\n",
                name.c_str(), points, admitted, check.statistics().kernelPasses);
}

}

TEST(ExactPointCheck, TheVerdictAndTheExcessesAreTheVerifiers)
{
    const std::vector<std::string> files = publishedProblems();
    if (files.empty()) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }

    std::size_t compared = 0;
    for (const std::string & file : files) {
        const std::string name = std::filesystem::path(file).filename().string();
        ProjectController project;
        project.load(file);
        if (project.templates().size() != project.omega()->values()->size()) {
            continue;
        }
        LtiSystem * structure = project.controllerStructure();
        ASSERT_NE(structure, nullptr) << name;
        const SpecificationSet specifications = toSpecificationSet(*project.specifications());

        ExactPointCheck check(*project.plant(), structure, *project.omega()->values(),
                              project.templates(), specifications);
        ASSERT_TRUE(check.usable()) << name;
        expectTheVerifiersEntries(check, structure, project, specifications, name, 2000);
        ++compared;
    }
    EXPECT_GE(compared, 10u);
}

TEST(ExactPointCheck, AStructureWithADelayIsEvaluatedAsTheVerifierDoes)
{
    const std::string file = (std::filesystem::path(QFTBX_EXAMPLES_DIR) / "toolbox-2.qft").string();
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * structure = project.controllerStructure();
    ASSERT_NE(structure, nullptr);
    std::unique_ptr<LtiSystem> delayed = structure->create("delayed", structure->numerator(), structure->denominator(),
                                                           structure->gain(), Parameter(0.02));
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());

    ExactPointCheck check(*project.plant(), delayed.get(), *project.omega()->values(),
                          project.templates(), specifications);
    ASSERT_TRUE(check.usable());
    expectTheVerifiersEntries(check, delayed.get(), project, specifications, "toolbox-2 delayed", 1000);
}

TEST(ExactPointCheck, TheToleranceRefusesTheEdgeTheVerifierAccepts)
{
    const std::string file = (std::filesystem::path(QFTBX_EXAMPLES_DIR) / "toolbox-2.qft").string();
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * structure = project.controllerStructure();
    LtiSystem * design = project.loopShapingResult()->controller();
    ASSERT_NE(structure, nullptr);
    ASSERT_NE(design, nullptr);
    const std::vector<double> & omega = *project.omega()->values();
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());

    PointController point;
    for (const Parameter & z : design->numerator()) point.zeros.push_back(z.nominal());
    for (const Parameter & p : design->denominator()) point.poles.push_back(p.nominal());

    const auto worstExcessAt = [&](double gain) {
        point.gain = gain;
        return checkAgainstSpecifications(*systemFromPoint(structure, point), *project.plant(), omega,
                                          project.templates(), specifications).worstExcessDb;
    };

    double allowed = design->gain().nominal();
    double refused = 0.5 * allowed;
    ASSERT_LE(worstExcessAt(allowed), 0.0) << "the published design meets its specifications";
    ASSERT_GT(worstExcessAt(refused), 0.0) << "half the gain does not";
    for (int i = 0; i < 200 && refused != allowed; ++i) {
        const double middle = 0.5 * (allowed + refused);
        if (middle == allowed || middle == refused) {
            break;
        }
        (worstExcessAt(middle) <= 0.0 ? allowed : refused) = middle;
    }

    const double edge = worstExcessAt(allowed);
    std::printf("EXACT-POINT toolbox-2 edge: gain %.17g, worst excess %.3g dB\n", allowed, edge);
    EXPECT_LE(edge, 0.0) << "the verifier accepts the last gain on the allowed side";
    ASSERT_GT(edge, -ExactPointCheck::kToleranceDb) << "the bisection has to land inside the tolerance for this to mean anything";

    ExactPointCheck check(*project.plant(), structure, omega, project.templates(), specifications);
    point.gain = allowed;
    EXPECT_FALSE(check.admits(point)) << "the check refuses what a rounding could turn into a violation";
    point.gain = allowed * (1.0 + 1e-9);
    EXPECT_TRUE(check.admits(point)) << "a hair inside, and it is admitted";
}

TEST(ExactPointCheck, WithoutAValueSetForEveryFrequencyItIsUnusable)
{
    const std::string file = (std::filesystem::path(QFTBX_EXAMPLES_DIR) / "toolbox-2.qft").string();
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "no published problems under " << QFTBX_EXAMPLES_DIR;
    }
    ProjectController project;
    project.load(file);
    LtiSystem * structure = project.controllerStructure();
    ASSERT_NE(structure, nullptr);
    const SpecificationSet specifications = toSpecificationSet(*project.specifications());

    CloudSet fewer = project.templates();
    fewer.pop_back();
    ExactPointCheck check(*project.plant(), structure, *project.omega()->values(), fewer, specifications);
    EXPECT_FALSE(check.usable());
    EXPECT_THROW(check.admits(cornerOf(structure, true)), InvalidInput);

    ExactPointCheck noStructure(*project.plant(), nullptr, *project.omega()->values(), project.templates(), specifications);
    EXPECT_FALSE(noStructure.usable());
}
