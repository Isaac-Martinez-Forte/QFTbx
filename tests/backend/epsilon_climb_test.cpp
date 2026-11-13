/**
 * @file
 * @brief The epsilons the proposal tries.
 *
 * The rounding takes a value up to three significant figures; the climb
 * goes one per cent at a time in three figures, never repeating a value,
 * and ends at the diameter; the rungs are at most four steps of 1.5 below
 * the diameter. An engine handed proposals made elsewhere returns them as
 * its own.
 */

#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "src/core/templates/epsilon_climb.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;

TEST(EpsilonClimb, TheRoundingGoesUpToThreeFigures)
{
    EXPECT_EQ(roundedUpToThreeFigures(1.2341), 1.24);
    EXPECT_EQ(roundedUpToThreeFigures(0.00123), 0.00123);
    EXPECT_EQ(roundedUpToThreeFigures(98761.0), 98800.0);
    EXPECT_EQ(roundedUpToThreeFigures(0.0), 0.0);
}

TEST(EpsilonClimb, TheClimbGoesOnePerCentAtATimeToTheDiameter)
{
    EpsilonClimb climb(1.0, 1.05);
    std::vector<double> candidates;
    while (const std::optional<double> candidate = climb.next()) {
        candidates.push_back(*candidate);
    }
    EXPECT_EQ(candidates, (std::vector<double>{1.0, 1.01, 1.03, 1.04, 1.05}));

    EpsilonClimb tight(2.0, 2.0);
    ASSERT_EQ(tight.next(), 2.0);
    EXPECT_FALSE(tight.next().has_value());
}

TEST(EpsilonClimb, TheRungsAreFourStepsOfOneAndAHalfBelowTheDiameter)
{
    EXPECT_EQ(rungCandidates(1.0, 100.0), (std::vector<double>{1.5, 2.25, 3.38, 5.07}));
    EXPECT_EQ(rungCandidates(1.0, 3.0), (std::vector<double>{1.5, 2.25}));
    EXPECT_TRUE(rungCandidates(1.0, 1.2).empty());
}

TEST(EpsilonClimb, AnEngineReturnsTheProposalsItIsHanded)
{
    TemplateEngine engine;
    engine.setClouds({{{0.0, 0.0}, {1.0, 0.0}}});
    EpsilonProposal proposal;
    proposal.connected = 1.0;
    proposal.epsilon = 1.5;
    proposal.diameter = 1.0;
    proposal.closes = true;
    engine.adoptProposals({proposal});
    const std::vector<EpsilonProposal> proposals = engine.proposeEpsilon();
    ASSERT_EQ(proposals.size(), 1u);
    EXPECT_EQ(proposals[0].epsilon, 1.5);
    EXPECT_TRUE(proposals[0].closes);
}
