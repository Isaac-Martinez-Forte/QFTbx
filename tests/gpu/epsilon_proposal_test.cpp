/**
 * @file
 * @brief The epsilon proposed on the card is the processor's.
 *
 * Without a card, the code every thread runs is run on the processor, on
 * the examples' clouds in both planes and on the clouds of walk_cases.h,
 * for a contour walked and for the alpha-shape: it must propose what the
 * template engine proposes, exactly. One of the lattices does not close at
 * the first epsilon of the climb, so the later rounds are run too. With a
 * card, its proposals are compared with the threads' code on the processor
 * and the GPU engine's with the processor's engine: the longest edge and
 * the diameter must agree, exactly in the complex plane, and whether the
 * walk closes; the epsilon comes from walks that may differ where two
 * candidates are almost equally good (contour_walk_test.cpp), so how many
 * are identical is printed.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "src/core/gpu/templates/epsilon_proposal.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/templates/epsilon_climb.h"
#include "src/core/templates/template_engine.h"
#include "tests/gpu/card.h"
#include "tests/gpu/walk_cases.h"

using namespace qftbx;

namespace {

std::vector<gputest::WalkCase> proposalCases()
{
    std::vector<gputest::WalkCase> cases = gputest::exampleCases({1.0});
    for (std::vector<gputest::WalkCase> more : {gputest::smallCases(), gputest::fallbackCases()}) {
        for (gputest::WalkCase & c : more) {
            cases.push_back(std::move(c));
        }
    }
    return cases;
}

std::vector<EpsilonProposal> engineProposals(const gputest::WalkCase & c, bool walked)
{
    TemplateEngine engine;
    engine.setHullMetric(c.metric, 1.0);
    engine.setAlphaShapeContour(!walked);
    engine.setClouds(c.clouds);
    return engine.proposeEpsilon();
}

std::string named(const gputest::WalkCase & c, bool walked)
{
    return c.name + (walked ? ", walked" : ", alpha-shape");
}

void expectSame(const std::vector<EpsilonProposal> & expected, const std::vector<EpsilonProposal> & found,
                const std::string & where)
{
    ASSERT_EQ(found.size(), expected.size()) << where;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string at = where + ", frequency " + std::to_string(i);
        EXPECT_EQ(found[i].connected, expected[i].connected) << at;
        EXPECT_EQ(found[i].diameter, expected[i].diameter) << at;
        EXPECT_EQ(found[i].epsilon, expected[i].epsilon) << at;
        EXPECT_EQ(found[i].closes, expected[i].closes) << at;
    }
}

struct Equivalence
{
    std::size_t proposals = 0;
    std::size_t identical = 0;
};

void expectEquivalent(const gputest::WalkCase & c, const std::vector<EpsilonProposal> & expected,
                      const std::vector<EpsilonProposal> & found, const std::string & where, Equivalence & tally)
{
    ASSERT_EQ(found.size(), expected.size()) << where;
    const double tolerance = c.metric == HullMetric::ComplexPlane ? 0.0 : 1e-12;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const std::string at = where + ", frequency " + std::to_string(i);
        ++tally.proposals;
        EXPECT_LE(std::abs(found[i].connected - expected[i].connected), tolerance * expected[i].connected) << at;
        EXPECT_LE(std::abs(found[i].diameter - expected[i].diameter), tolerance * expected[i].diameter) << at;
        EXPECT_EQ(found[i].closes, expected[i].closes) << at;
        if (found[i].epsilon == expected[i].epsilon) {
            ++tally.identical;
        }
    }
}

}

TEST(ProposalOnTheCard, TheThreadsCodeProposesAsTheEngine)
{
    std::size_t checked = 0;
    for (const gputest::WalkCase & c : proposalCases()) {
        for (const bool walked : {true, false}) {
            expectSame(engineProposals(c, walked), gpu::proposedOnProcessor(c.clouds, c.metric, 1.0, walked),
                       named(c, walked));
            ++checked;
        }
    }
    EXPECT_EQ(checked, 2 * (2 * gputest::exampleFiles().size() + gputest::smallCases().size()
                            + gputest::fallbackCases().size()));
}

TEST(ProposalOnTheCard, TheClimbGoesPastTheFirstEpsilonAsTheEngine)
{
    const gputest::WalkCase c = gputest::fallbackCases().front();
    const std::vector<EpsilonProposal> expected = engineProposals(c, true);
    ASSERT_EQ(expected.size(), 1u);

    bool fellBack = false;
    bool truncated = false;
    TemplateEngine engine;
    engine.epsilonHull(c.clouds[0], roundedUpToThreeFigures(expected[0].connected), &fellBack, &truncated);
    EXPECT_TRUE(fellBack);
    EXPECT_TRUE(expected[0].closes);
    EXPECT_GT(expected[0].epsilon, 1.5 * expected[0].connected);

    expectSame(expected, gpu::proposedOnProcessor(c.clouds, c.metric, 1.0, true), c.name);
}

TEST(GpuTemplates, TheCardProposesAsTheThreadsCodeOnTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    Equivalence tally;
    std::size_t rounds = 0;
    for (const gputest::WalkCase & c : proposalCases()) {
        const auto resident = gpu::cloudsOnCard(c.clouds);
        if (resident == nullptr) {
            continue;
        }
        for (const bool walked : {true, false}) {
            const gpu::ProposedOnCard proposed = gpu::proposedOnCard(*resident, c.metric, 1.0, walked, nullptr);
            EXPECT_FALSE(proposed.cancelled) << named(c, walked);
            rounds = std::max(rounds, proposed.rounds);
            expectEquivalent(c, gpu::proposedOnProcessor(c.clouds, c.metric, 1.0, walked), proposed.proposals,
                             named(c, walked), tally);
        }
    }
    EXPECT_GE(rounds, 3u);
    std::printf("epsilons proposed on the card identical to the threads' code on the processor: %zu of %zu\n",
                tally.identical, tally.proposals);
}

TEST(GpuTemplates, TheEngineOfTheCardProposesAsTheProcessor)
{
    QFTBX_REQUIRE_CARD();

    Equivalence tally;
    for (const gputest::WalkCase & c : gputest::exampleCases({1.0})) {
        gpu::GpuTemplateEngine card;
        card.setHullMetric(c.metric, 1.0);
        card.setClouds(c.clouds);
        const std::vector<EpsilonProposal> proposed = card.proposeEpsilon();
        expectEquivalent(c, engineProposals(c, true), proposed, c.name, tally);
        EXPECT_NE(card.resident(), nullptr) << c.name;

        card.setAlphaShapeContour(true);
        expectEquivalent(c, engineProposals(c, false), card.proposeEpsilon(), c.name + ", alpha-shape", tally);
    }
    std::printf("epsilons of the GPU engine identical to the processor's: %zu of %zu\n", tally.identical,
                tally.proposals);
}
