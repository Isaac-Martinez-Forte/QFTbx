/**
 * @file
 * @brief The clouds prepared for the walk on the card give the processor's.
 *
 * Without a card, the code every thread runs is run on the processor: on
 * the clouds of the 27 examples, in the complex plane at the project's
 * epsilon and on the Nichols plane at the one the engine proposes there,
 * each also at half of it, where the clouds split into parts, and on small
 * clouds with repeats, a single point, parts that reach equally far, a cut
 * of the phase and an epsilon too small for the grid. It must give the
 * processor's points, projection and parts exactly. With a card, the
 * card's preparation must be the processor's run of the same code: the
 * same points in the same order and the same parts, and the projection on
 * the Nichols plane within 1e-12, since the card's arc tangent and
 * logarithm are not the processor's; and the engine of the card, given
 * clouds from a file, walks them into the processor's contours.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <filesystem>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/templates/template_engine.h"
#include "src/core/templates/walk_cloud.h"
#include "tests/gpu/card.h"

using namespace qftbx;

namespace {

constexpr double kProjectionTolerance = 1e-12;

std::vector<std::filesystem::path> examples()
{
    std::vector<std::filesystem::path> files;
    for (const auto & entry : std::filesystem::directory_iterator(QFTBX_EXAMPLES_DIR)) {
        if (entry.path().extension() == ".qft") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<PreparedCloud> onTheProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                          HullMetric metric, double dbPerDegree)
{
    std::vector<PreparedCloud> prepared;
    for (std::size_t i = 0; i < clouds.size(); ++i) {
        PreparedCloud one;
        one.sorted = sortedCloud(clouds[i], metric, dbPerDegree);
        const NeighbourGrid grid(one.sorted.walk, epsilon[i]);
        one.parts = cloudParts(one.sorted.walk, epsilon[i], grid);
        prepared.push_back(std::move(one));
    }
    return prepared;
}

void expectSame(const std::vector<PreparedCloud> & expected, const std::vector<PreparedCloud> & found,
                double tolerance, const std::string & where)
{
    ASSERT_EQ(found.size(), expected.size()) << where;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string at = where + ", frequency " + std::to_string(i);
        EXPECT_EQ(found[i].sorted.points, expected[i].sorted.points) << at;
        ASSERT_EQ(found[i].sorted.walk.size(), expected[i].sorted.walk.size()) << at;
        double largest = 0.0;
        for (std::size_t k = 0; k < expected[i].sorted.walk.size(); ++k) {
            const std::complex<double> e = expected[i].sorted.walk[k];
            largest = std::max(largest, std::abs(found[i].sorted.walk[k] - e) / (1.0 + std::abs(e)));
        }
        EXPECT_LE(largest, tolerance) << at;
        EXPECT_EQ(found[i].parts.members, expected[i].parts.members) << at;
        EXPECT_EQ(found[i].parts.starts, expected[i].parts.starts) << at;
    }
}

struct Case
{
    std::string name;
    CloudSet clouds;
    std::vector<double> epsilon;
    HullMetric metric;
};

std::vector<Case> exampleCases()
{
    std::vector<Case> cases;
    for (const std::filesystem::path & file : examples()) {
        ProjectController project;
        project.load(file.string());
        const CloudSet clouds = project.templates();
        const std::vector<double> projectEpsilon = *project.epsilon();

        TemplateEngine proposing;
        proposing.setHullMetric(HullMetric::Nichols, 1.0);
        proposing.setClouds(clouds);
        std::vector<double> nicholsEpsilon;
        for (const EpsilonProposal & proposal : proposing.proposeEpsilon()) {
            nicholsEpsilon.push_back(proposal.epsilon);
        }

        for (const double scale : {1.0, 0.5}) {
            std::vector<double> complexAt = projectEpsilon;
            std::vector<double> nicholsAt = nicholsEpsilon;
            for (double & e : complexAt) {
                e *= scale;
            }
            for (double & e : nicholsAt) {
                e *= scale;
            }
            const std::string suffix = " x" + std::to_string(scale);
            cases.push_back({file.stem().string() + " complex" + suffix, clouds, complexAt, HullMetric::ComplexPlane});
            cases.push_back({file.stem().string() + " Nichols" + suffix, clouds, nicholsAt, HullMetric::Nichols});
        }
    }
    return cases;
}

void addBlock(ComplexCloud & cloud, std::complex<double> corner, int side)
{
    for (int i = 0; i < side; ++i) {
        for (int j = 0; j < side; ++j) {
            cloud.push_back(corner + std::complex<double>(i, j));
        }
    }
}

std::vector<Case> smallCases()
{
    ComplexCloud repeats{{3.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {2.0, 0.0}, {3.0, 0.0}};
    ComplexCloud blocks;
    addBlock(blocks, {0.0, 10.0}, 3);
    addBlock(blocks, {5.0, 0.0}, 2);
    addBlock(blocks, {0.0, 0.0}, 3);
    ComplexCloud acrossTheCut;
    for (int k = -6; k <= 6; ++k) {
        acrossTheCut.push_back(std::polar(1.0 + 0.1 * (k + 6), 3.1 + 0.01 * k));
        acrossTheCut.push_back(std::polar(2.0 + 0.1 * (k + 6), -3.1 + 0.01 * k));
    }
    ComplexCloud spread{{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {1e-12, 0.0}};

    return {
        {"repeats", {repeats}, {1.5}, HullMetric::ComplexPlane},
        {"repeats on Nichols", {repeats}, {40.0}, HullMetric::Nichols},
        {"one point", {{{2.0, -1.0}}}, {1.0}, HullMetric::ComplexPlane},
        {"parts that reach equally far", {blocks}, {1.5}, HullMetric::ComplexPlane},
        {"two frequencies", {blocks, repeats}, {1.5, 0.5}, HullMetric::ComplexPlane},
        {"across the cut", {acrossTheCut}, {3.0}, HullMetric::Nichols},
        {"an epsilon too small for the grid", {spread}, {1e-12}, HullMetric::ComplexPlane},
    };
}

}

TEST(WalkOnTheCard, TheThreadsCodePreparesTheExamplesAsTheProcessor)
{
    std::size_t checked = 0;
    for (const Case & c : exampleCases()) {
        expectSame(onTheProcessor(c.clouds, c.epsilon, c.metric, 1.0),
                   gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), 0.0, c.name);
        ++checked;
    }
    EXPECT_EQ(checked, 4 * examples().size());
}

TEST(WalkOnTheCard, TheThreadsCodePreparesSmallCloudsAsTheProcessor)
{
    for (const Case & c : smallCases()) {
        expectSame(onTheProcessor(c.clouds, c.epsilon, c.metric, 2.0),
                   gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 2.0), 0.0, c.name);
    }
}

TEST(GpuTemplates, TheCardPreparesTheCloudsAsTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    std::vector<Case> cases = exampleCases();
    for (Case & c : smallCases()) {
        cases.push_back(std::move(c));
    }
    for (const Case & c : cases) {
        const auto resident = gpu::cloudsOnCard(c.clouds);
        ASSERT_NE(resident, nullptr) << c.name;
        const gpu::PreparedOnCard prepared = gpu::preparedOnCard(*resident, c.epsilon, c.metric, 1.0, nullptr);
        EXPECT_FALSE(prepared.cancelled) << c.name;
        expectSame(gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), prepared.clouds,
                   c.metric == HullMetric::Nichols ? kProjectionTolerance : 0.0, c.name);
    }
}

TEST(GpuTemplates, TheEngineOfTheCardWalksCloudsFromAFileAsTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    for (const Case & c : exampleCases()) {
        TemplateEngine processor;
        processor.setHullMetric(c.metric, 1.0);
        processor.setClouds(c.clouds);
        processor.computeContours(c.epsilon);

        gpu::GpuTemplateEngine card;
        card.setHullMetric(c.metric, 1.0);
        card.setClouds(c.clouds);
        card.computeContours(c.epsilon);

        EXPECT_EQ(card.contours(), processor.contours()) << c.name;
        ASSERT_EQ(card.contourReports().size(), processor.contourReports().size()) << c.name;
        for (std::size_t i = 0; i < processor.contourReports().size(); ++i) {
            EXPECT_EQ(card.contourReports()[i].componentStarts, processor.contourReports()[i].componentStarts)
                    << c.name << ", frequency " << i;
        }
        EXPECT_NE(card.resident(), nullptr) << c.name;
    }
}
