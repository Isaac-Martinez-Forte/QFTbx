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
 * logarithm are not the processor's. The clouds are those of walk_cases.h.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>
#include <vector>

#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/templates/walk_cloud.h"
#include "tests/gpu/card.h"
#include "tests/gpu/walk_cases.h"

using namespace qftbx;

namespace {

constexpr double kProjectionTolerance = 1e-12;

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

}

TEST(WalkOnTheCard, TheThreadsCodePreparesTheExamplesAsTheProcessor)
{
    std::size_t checked = 0;
    for (const gputest::WalkCase & c : gputest::exampleCases({1.0, 0.5})) {
        expectSame(onTheProcessor(c.clouds, c.epsilon, c.metric, 1.0),
                   gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), 0.0, c.name);
        ++checked;
    }
    EXPECT_EQ(checked, 4 * gputest::exampleFiles().size());
}

TEST(WalkOnTheCard, TheThreadsCodePreparesSmallCloudsAsTheProcessor)
{
    for (const gputest::WalkCase & c : gputest::smallCases()) {
        expectSame(onTheProcessor(c.clouds, c.epsilon, c.metric, 2.0),
                   gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 2.0), 0.0, c.name);
    }
}

TEST(GpuTemplates, TheCardPreparesTheCloudsAsTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    std::vector<gputest::WalkCase> cases = gputest::exampleCases({1.0, 0.5});
    for (gputest::WalkCase & c : gputest::smallCases()) {
        cases.push_back(std::move(c));
    }
    for (const gputest::WalkCase & c : cases) {
        const auto resident = gpu::cloudsOnCard(c.clouds);
        ASSERT_NE(resident, nullptr) << c.name;
        const gpu::PreparedOnCard prepared = gpu::preparedOnCard(*resident, c.epsilon, c.metric, 1.0, nullptr);
        EXPECT_FALSE(prepared.cancelled) << c.name;
        expectSame(gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), prepared.clouds,
                   c.metric == HullMetric::Nichols ? kProjectionTolerance : 0.0, c.name);
    }
}
