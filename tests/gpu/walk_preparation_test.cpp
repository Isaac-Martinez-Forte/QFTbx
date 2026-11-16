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
 * card's preparation must be the processor's run of the same code up to
 * the card's square root, arc tangent and logarithm, which are not the
 * processor's: the same points, in an order whose magnitudes agree place
 * by place to their last bits (points of almost the same magnitude may
 * change places), each projected within 1e-12 on the Nichols plane, and
 * the same parts as sets of points, whatever their order. The clouds are
 * those of walk_cases.h, those of one size, the only ones the card holds.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <map>
#include <string>
#include <utility>
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

using Key = std::pair<double, double>;

Key keyOf(const std::complex<double> & z)
{
    return {z.real(), z.imag()};
}

std::vector<std::vector<Key>> partsOf(const PreparedCloud & cloud)
{
    std::vector<std::vector<Key>> parts;
    const CloudParts & found = cloud.parts;
    for (std::size_t p = 0; p < found.starts.size(); ++p) {
        const std::size_t end = p + 1 < found.starts.size() ? found.starts[p + 1] : found.members.size();
        std::vector<Key> part;
        for (std::size_t k = found.starts[p]; k < end; ++k) {
            part.push_back(keyOf(cloud.sorted.points[static_cast<std::size_t>(found.members[k])]));
        }
        std::sort(part.begin(), part.end());
        parts.push_back(std::move(part));
    }
    std::sort(parts.begin(), parts.end());
    return parts;
}

void expectEquivalent(const std::vector<PreparedCloud> & expected, const std::vector<PreparedCloud> & found,
                      double tolerance, const std::string & where)
{
    ASSERT_EQ(found.size(), expected.size()) << where;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string at = where + ", frequency " + std::to_string(i);
        const ComplexCloud & points = expected[i].sorted.points;
        ASSERT_EQ(found[i].sorted.points.size(), points.size()) << at;

        std::map<Key, std::complex<double>> walkOf;
        for (std::size_t k = 0; k < points.size(); ++k) {
            walkOf[keyOf(points[k])] = expected[i].sorted.walk[k];
        }
        double largestMagnitude = 0.0;
        double largestWalk = 0.0;
        for (std::size_t k = 0; k < points.size(); ++k) {
            const std::complex<double> z = found[i].sorted.points[k];
            const auto known = walkOf.find(keyOf(z));
            ASSERT_NE(known, walkOf.end()) << at << ": a point the processor does not have";
            largestMagnitude = std::max(largestMagnitude,
                                        std::abs(std::abs(z) - std::abs(points[k])) / std::max(std::abs(z), 1e-300));
            largestWalk = std::max(largestWalk,
                                   std::abs(found[i].sorted.walk[k] - known->second) / (1.0 + std::abs(known->second)));
        }
        EXPECT_LE(largestMagnitude, 1e-14) << at;
        EXPECT_LE(largestWalk, tolerance) << at;
        EXPECT_EQ(partsOf(found[i]), partsOf(expected[i])) << at;
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
        if (!gputest::oneSize(c.clouds)) {
            continue;
        }
        const auto resident = gpu::cloudsOnCard(c.clouds);
        ASSERT_NE(resident, nullptr) << c.name;
        const gpu::PreparedOnCard prepared = gpu::preparedOnCard(*resident, c.epsilon, c.metric, 1.0, nullptr);
        EXPECT_FALSE(prepared.cancelled) << c.name;
        expectEquivalent(gpu::preparedOnProcessor(c.clouds, c.epsilon, c.metric, 1.0), prepared.clouds,
                         c.metric == HullMetric::Nichols ? kProjectionTolerance : 0.0, c.name);
    }
}
