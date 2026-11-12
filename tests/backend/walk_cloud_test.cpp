/**
 * @file
 * @brief The cloud prepared for the walk: its order, its parts, and the
 * walk of a cloud prepared elsewhere.
 *
 * The points come out in MATLAB's complex order without repeats, and the
 * projection onto the plane of the walk is the engine's. The parts cover
 * every point once, each in the order of the points, the rightmost part
 * first and, between parts that reach equally far, the one whose first
 * point comes first. An engine handed the prepared clouds of every
 * frequency walks them into the contours it would have walked itself, on
 * either plane and with the cloud split into parts, and refuses prepared
 * clouds that are not one per frequency; handed the walks of every
 * frequency, it finishes them as its own, the whole cloud standing in where
 * none closed, or the error when it may not.
 */

#include <gtest/gtest.h>

#include <complex>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/templates/template_engine.h"
#include "src/core/templates/walk_cloud.h"

using namespace qftbx;

namespace {

void addBlock(ComplexCloud & cloud, std::complex<double> corner, int side)
{
    for (int i = 0; i < side; ++i) {
        for (int j = 0; j < side; ++j) {
            cloud.push_back(corner + std::complex<double>(i, j));
        }
    }
}

std::vector<PreparedCloud> preparedFor(const CloudSet & clouds, const std::vector<double> & epsilon, HullMetric metric)
{
    std::vector<PreparedCloud> prepared;
    for (std::size_t i = 0; i < clouds.size(); ++i) {
        PreparedCloud one;
        one.sorted = sortedCloud(clouds[i], metric, 1.0);
        const NeighbourGrid grid(one.sorted.walk, epsilon[i]);
        one.parts = cloudParts(one.sorted.walk, epsilon[i], grid);
        prepared.push_back(std::move(one));
    }
    return prepared;
}

}

TEST(WalkCloud, ThePointsAreInMatlabOrderWithoutRepeats)
{
    const ComplexCloud cloud{{3.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {2.0, 0.0}, {3.0, 0.0}};
    const SortedCloud sorted = sortedCloud(cloud, HullMetric::ComplexPlane, 1.0);

    const ComplexCloud expected{{1.0, 0.0}, {0.0, 1.0}, {-1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}};
    EXPECT_EQ(sorted.points, expected);
    EXPECT_EQ(sorted.walk, expected);

    const SortedCloud nichols = sortedCloud(cloud, HullMetric::Nichols, 2.0);
    EXPECT_EQ(nichols.points, expected);
    EXPECT_EQ(nichols.walk, projectedCloud(expected, HullMetric::Nichols, 2.0));
}

TEST(WalkCloud, ThePartsGoRightmostFirstAndTheFirstPointBreaksATie)
{
    ComplexCloud cloud;
    addBlock(cloud, {0.0, 10.0}, 3);
    addBlock(cloud, {5.0, 0.0}, 2);
    addBlock(cloud, {0.0, 0.0}, 3);
    const SortedCloud sorted = sortedCloud(cloud, HullMetric::ComplexPlane, 1.0);
    const NeighbourGrid grid(sorted.walk, 1.5);
    const CloudParts parts = cloudParts(sorted.walk, 1.5, grid);

    ASSERT_EQ(parts.starts.size(), 3u);
    ASSERT_EQ(parts.members.size(), sorted.points.size());
    const auto partOf = [&](std::size_t p) {
        const std::size_t end = p + 1 < parts.starts.size() ? parts.starts[p + 1] : parts.members.size();
        return std::vector<std::int32_t>(parts.members.begin() + static_cast<std::ptrdiff_t>(parts.starts[p]),
                                         parts.members.begin() + static_cast<std::ptrdiff_t>(end));
    };
    std::vector<int> seen(sorted.points.size(), 0);
    for (std::size_t p = 0; p < parts.starts.size(); ++p) {
        const std::vector<std::int32_t> members = partOf(p);
        for (std::size_t m = 0; m < members.size(); ++m) {
            ++seen[static_cast<std::size_t>(members[m])];
            if (m > 0) {
                EXPECT_LT(members[m - 1], members[m]);
            }
        }
    }
    EXPECT_EQ(seen, std::vector<int>(sorted.points.size(), 1));

    const auto rightmost = [&](std::size_t p) {
        double best = -1e300;
        for (const std::int32_t index : partOf(p)) {
            best = std::max(best, sorted.walk[static_cast<std::size_t>(index)].real());
        }
        return best;
    };
    EXPECT_EQ(rightmost(0), 6.0);
    EXPECT_EQ(rightmost(1), 2.0);
    EXPECT_EQ(rightmost(2), 2.0);
    EXPECT_EQ(sorted.points[static_cast<std::size_t>(partOf(1).front())], std::complex<double>(0.0, 0.0));
    EXPECT_EQ(sorted.points[static_cast<std::size_t>(partOf(2).front())], std::complex<double>(0.0, 10.0));
}

TEST(WalkCloud, TheEngineWalksTheCloudsPreparedElsewhereAsItWalksItsOwn)
{
    ProjectController project;
    project.load(std::string(QFTBX_EXAMPLES_DIR) + "/dcm-AC.qft");
    const CloudSet clouds = project.templates();

    for (const HullMetric metric : {HullMetric::ComplexPlane, HullMetric::Nichols}) {
        TemplateEngine proposing;
        proposing.setHullMetric(metric, 1.0);
        proposing.setClouds(clouds);
        const std::vector<EpsilonProposal> proposals = proposing.proposeEpsilon();
        for (const double scale : {1.0, 0.5}) {
            std::vector<double> epsilon;
            for (const EpsilonProposal & proposal : proposals) {
                epsilon.push_back(proposal.epsilon * scale);
            }
            const std::string where = std::string(metric == HullMetric::Nichols ? "Nichols" : "complex")
                    + " x" + std::to_string(scale);

            TemplateEngine own;
            own.setHullMetric(metric, 1.0);
            own.setClouds(clouds);
            own.computeContours(epsilon);

            TemplateEngine handed;
            handed.setHullMetric(metric, 1.0);
            handed.setClouds(clouds);
            handed.setEpsilon(epsilon);
            handed.contourPrepared(preparedFor(clouds, epsilon, metric));

            EXPECT_EQ(handed.contours(), own.contours()) << where;
            ASSERT_EQ(handed.contourReports().size(), own.contourReports().size()) << where;
            bool split = false;
            for (std::size_t i = 0; i < own.contourReports().size(); ++i) {
                EXPECT_EQ(handed.contourReports()[i].componentStarts, own.contourReports()[i].componentStarts) << where;
                split = split || own.contourReports()[i].components > 1;
            }
            if (scale < 1.0) {
                EXPECT_TRUE(split) << where;
            }
        }
    }
}

TEST(WalkCloud, PreparedCloudsThatAreNotOnePerFrequencyAreRefused)
{
    ProjectController project;
    project.load(std::string(QFTBX_EXAMPLES_DIR) + "/dcm-AC.qft");
    const CloudSet clouds = project.templates();
    std::vector<PreparedCloud> prepared = preparedFor(clouds, *project.epsilon(), HullMetric::ComplexPlane);
    prepared.pop_back();

    TemplateEngine engine;
    engine.setClouds(clouds);
    engine.setEpsilon(*project.epsilon());
    EXPECT_THROW(engine.contourPrepared(prepared), ComputationError);
}

TEST(WalkCloud, TheEngineFinishesTheWalksMadeElsewhereAsItFinishesItsOwn)
{
    ProjectController project;
    project.load(std::string(QFTBX_EXAMPLES_DIR) + "/dcm-AC.qft");
    const CloudSet clouds = project.templates();

    for (const double scale : {1.0, 0.5, 1e-6}) {
        std::vector<double> epsilon = *project.epsilon();
        for (double & e : epsilon) {
            e *= scale;
        }
        const std::string where = "x" + std::to_string(scale);

        TemplateEngine own;
        own.setClouds(clouds);
        own.computeContours(epsilon);

        TemplateEngine walking;
        std::vector<WalkedContour> walked(clouds.size());
        for (std::size_t i = 0; i < clouds.size(); ++i) {
            walked[i].contour = walking.epsilonHull(clouds[i], epsilon[i], &walked[i].fellBack, &walked[i].truncated,
                                                    &walked[i].starts);
        }

        TemplateEngine handed;
        handed.setClouds(clouds);
        handed.takeEpsilon(epsilon);
        handed.contourWalked(walked);

        EXPECT_EQ(handed.contours(), own.contours()) << where;
        ASSERT_EQ(handed.contourReports().size(), own.contourReports().size()) << where;
        for (std::size_t i = 0; i < own.contourReports().size(); ++i) {
            const ContourReport & a = handed.contourReports()[i];
            const ContourReport & b = own.contourReports()[i];
            EXPECT_EQ(a.componentStarts, b.componentStarts) << where;
            EXPECT_EQ(a.wholeCloud, b.wholeCloud) << where;
            EXPECT_EQ(a.contourPoints, b.contourPoints) << where;
        }
        if (scale < 1e-3) {
            EXPECT_TRUE(own.contourReports().front().wholeCloud) << where;

            TemplateEngine failing;
            failing.setWholeCloudStandsIn(false);
            failing.setClouds(clouds);
            failing.takeEpsilon(epsilon);
            EXPECT_THROW(failing.contourWalked(walked), ComputationError) << where;
        }
    }
}
