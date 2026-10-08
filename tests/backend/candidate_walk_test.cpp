/**
 * @file
 * @brief The candidates of an epsilon-small box are walked once each.
 *
 * With a zero, a pole and the gain uncertain, the walk starts with the
 * anti-blocking corner, the lower corner and the centre, and then visits the
 * other six of the eight corners, so every corner is asked once and none
 * twice. A caller that recomputes the gain walks the four corners of the
 * zero and the pole, the anti-blocking and the lower ones among the first
 * three candidates, and so asks each pair of a zero and a pole once. Past six
 * uncertain parameters only the first three candidates are walked, a fixed
 * parameter keeps its value at every candidate, and the walk stops at the
 * first candidate its visitor accepts.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/core/loopshaping/common/common_functions.h"
#include "src/core/loopshaping/common/point_controller.h"
#include "src/core/math/range.h"
#include "src/core/system/parameter.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;

namespace {

std::unique_ptr<LtiSystem> boxWith(std::size_t zeros, std::size_t poles)
{
    std::vector<Parameter> numerator, denominator;
    for (std::size_t i = 0; i < zeros; ++i) {
        numerator.emplace_back("z" + std::to_string(i), Range(1.0 + i, 2.0 + i), 1.0 + i);
    }
    for (std::size_t i = 0; i < poles; ++i) {
        denominator.emplace_back("p" + std::to_string(i), Range(10.0 + i, 20.0 + i), 10.0 + i);
    }
    return std::make_unique<ZeroPoleGain>(std::string("box"), numerator, denominator,
                                          Parameter(std::string("k"), Range(100.0, 200.0), 100.0), Parameter(0.0));
}

std::vector<PointController> walked(LtiSystem * box, CandidateGain gain)
{
    std::vector<PointController> visited;
    forEachCandidate(box, [&](const PointController & candidate) {
        visited.push_back(candidate);
        return false;
    }, gain);
    return visited;
}

bool same(const PointController & a, const PointController & b)
{
    return a.gain == b.gain && a.zeros == b.zeros && a.poles == b.poles;
}

}

TEST(CandidateWalk, EveryCornerOnceAfterTheAntiBlockingTheLowerAndTheCentre)
{
    const std::unique_ptr<LtiSystem> box = boxWith(1, 1);
    const std::vector<PointController> visited = walked(box.get(), CandidateGain::FromTheCorner);

    ASSERT_EQ(visited.size(), 9u);
    EXPECT_TRUE(same(visited[0], cornerOf(box.get(), false)));
    EXPECT_TRUE(same(visited[1], cornerOf(box.get(), true)));
    EXPECT_TRUE(same(visited[2], PointController{150.0, {1.5}, {15.0}}));

    for (std::size_t a = 0; a < visited.size(); ++a) {
        for (std::size_t b = a + 1; b < visited.size(); ++b) {
            EXPECT_FALSE(same(visited[a], visited[b])) << "candidates " << a << " and " << b;
        }
    }
    for (const double k : {100.0, 200.0}) {
        for (const double z : {1.0, 2.0}) {
            for (const double p : {10.0, 20.0}) {
                std::size_t found = 0;
                for (const PointController & candidate : visited) {
                    found += same(candidate, PointController{k, {z}, {p}}) ? 1 : 0;
                }
                EXPECT_EQ(found, 1u) << "corner k=" << k << " z=" << z << " p=" << p;
            }
        }
    }
}

TEST(CandidateWalk, ARecomputedGainWalksTheCornersOfTheZerosAndPolesOnce)
{
    const std::unique_ptr<LtiSystem> box = boxWith(1, 1);
    const std::vector<PointController> visited = walked(box.get(), CandidateGain::Recomputed);

    ASSERT_EQ(visited.size(), 5u);
    for (std::size_t a = 0; a < visited.size(); ++a) {
        for (std::size_t b = a + 1; b < visited.size(); ++b) {
            EXPECT_FALSE(visited[a].zeros == visited[b].zeros && visited[a].poles == visited[b].poles)
                << "candidates " << a << " and " << b;
        }
    }
    for (const double z : {1.0, 2.0}) {
        for (const double p : {10.0, 20.0}) {
            std::size_t found = 0;
            for (const PointController & candidate : visited) {
                found += candidate.zeros == std::vector<double>{z} && candidate.poles == std::vector<double>{p} ? 1 : 0;
            }
            EXPECT_EQ(found, 1u) << "corner z=" << z << " p=" << p;
        }
    }
}

TEST(CandidateWalk, PastSixUncertainParametersOnlyTheFirstThreeAreWalked)
{
    const std::unique_ptr<LtiSystem> six = boxWith(3, 2);
    EXPECT_EQ(walked(six.get(), CandidateGain::FromTheCorner).size(), 3u + 64u - 2u);
    EXPECT_EQ(walked(six.get(), CandidateGain::Recomputed).size(), 3u + 32u - 2u);

    const std::unique_ptr<LtiSystem> seven = boxWith(3, 3);
    EXPECT_EQ(walked(seven.get(), CandidateGain::FromTheCorner).size(), 3u);
    EXPECT_EQ(walked(seven.get(), CandidateGain::Recomputed).size(), 3u);
}

TEST(CandidateWalk, AFixedParameterKeepsItsValueAndTheWalkStopsAtTheFirstAccepted)
{
    const std::unique_ptr<LtiSystem> box = std::make_unique<ZeroPoleGain>(
        std::string("box"), std::vector<Parameter>{Parameter(3.0)},
        std::vector<Parameter>{Parameter(std::string("p"), Range(10.0, 20.0), 10.0)},
        Parameter(std::string("k"), Range(100.0, 200.0), 100.0), Parameter(0.0));

    const std::vector<PointController> visited = walked(box.get(), CandidateGain::FromTheCorner);
    ASSERT_EQ(visited.size(), 3u + 4u - 2u);
    for (const PointController & candidate : visited) {
        EXPECT_EQ(candidate.zeros, std::vector<double>{3.0});
    }

    std::size_t asked = 0;
    EXPECT_TRUE(forEachCandidate(box.get(), [&](const PointController &) { return ++asked == 2; }));
    EXPECT_EQ(asked, 2u);
}
