/**
 * @file
 * @brief Tests of the union of closed intervals the best-gain search intersects.
 *
 * The magnitudes a design frequency allows at one phase are a union of closed
 * intervals: one for an open boundary, two for a closed one, more for a
 * multivalued one. Held as a pair of numbers the set collapses to its hull and
 * the lower branch of a closed boundary, often the cheapest feasible gain,
 * disappears. The tests pin the canonical form, so that the count is the
 * number of components, the one-pass intersection, the shift, and the worked
 * case of the best-gain formalisation, where the smallest admissible gain lies
 * in the lower branch, 22 dB below the one a single-crossing formula returns.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "src/core/math/range_union.h"

using namespace qftbx;

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

}

TEST(RangeUnion, TheDefaultSetIsEmptyAndHasNoExtremes)
{
    const RangeUnion set;

    EXPECT_TRUE(set.isEmpty());
    EXPECT_EQ(set.count(), 0u);
    EXPECT_FALSE(set.contains(0.0));
    EXPECT_THROW(set.minimum(), std::domain_error);
    EXPECT_THROW(set.maximum(), std::domain_error);
}

TEST(RangeUnion, TheWholeLineHoldsEveryFiniteValue)
{
    const RangeUnion set = RangeUnion::whole();

    ASSERT_EQ(set.count(), 1u);
    EXPECT_TRUE(set.contains(-1e300));
    EXPECT_TRUE(set.contains(0.0));
    EXPECT_TRUE(set.contains(1e300));
    EXPECT_EQ(set.minimum(), -kInfinity);
    EXPECT_EQ(set.maximum(), kInfinity);
}

TEST(RangeUnion, AnIntervalGivenInvertedIsEmpty)
{
    EXPECT_TRUE(RangeUnion::of(5.0, 3.0).isEmpty());
    EXPECT_EQ(RangeUnion::of(3.0, 3.0).count(), 1u);
}

TEST(RangeUnion, ContainmentIncludesTheEnds)
{
    const RangeUnion set = RangeUnion::of(-2.0, 4.0);

    EXPECT_TRUE(set.contains(-2.0));
    EXPECT_TRUE(set.contains(4.0));
    EXPECT_FALSE(set.contains(-2.001));
    EXPECT_FALSE(set.contains(4.001));
}

TEST(RangeUnion, UnsortedOverlappingAndTouchingMembersComeOutCanonical)
{
    const std::vector<double> lo{5.0, -1.0, 7.0, 10.0, 6.0};
    const std::vector<double> hi{8.0, 0.0, 10.0, 12.0, 7.0};

    const RangeUnion set = RangeUnion::of(lo.data(), hi.data(), lo.size());

    ASSERT_EQ(set.count(), 2u);
    EXPECT_DOUBLE_EQ(set.at(0).min, -1.0);
    EXPECT_DOUBLE_EQ(set.at(0).max, 0.0);
    EXPECT_DOUBLE_EQ(set.at(1).min, 5.0);
    EXPECT_DOUBLE_EQ(set.at(1).max, 12.0);
}

TEST(RangeUnion, CountIsTheNumberOfComponents)
{
    const std::vector<double> lo{-kInfinity, 12.0};
    const std::vector<double> hi{-6.0, kInfinity};

    const RangeUnion set = RangeUnion::of(lo.data(), hi.data(), lo.size());

    EXPECT_EQ(set.count(), 2u);
    EXPECT_FALSE(set.contains(0.0));
    EXPECT_TRUE(set.contains(-6.0));
    EXPECT_TRUE(set.contains(12.0));
}

TEST(RangeUnion, DisjointSetsIntersectToNothing)
{
    RangeUnion set = RangeUnion::of(0.0, 1.0);
    set.intersectWith(RangeUnion::of(2.0, 3.0));

    EXPECT_TRUE(set.isEmpty());
}

TEST(RangeUnion, SetsThatOnlyTouchIntersectInThatPoint)
{
    RangeUnion set = RangeUnion::of(0.0, 1.0);
    set.intersectWith(RangeUnion::of(1.0, 2.0));

    ASSERT_EQ(set.count(), 1u);
    EXPECT_DOUBLE_EQ(set.minimum(), 1.0);
    EXPECT_DOUBLE_EQ(set.maximum(), 1.0);
}

TEST(RangeUnion, IntersectingTwoBranchesWithOneIntervalKeepsBoth)
{
    const std::vector<double> lo{-kInfinity, 12.0};
    const std::vector<double> hi{-6.0, kInfinity};

    RangeUnion set = RangeUnion::of(lo.data(), hi.data(), lo.size());
    set.intersectWith(-20.0, 20.0);

    ASSERT_EQ(set.count(), 2u);
    EXPECT_DOUBLE_EQ(set.at(0).min, -20.0);
    EXPECT_DOUBLE_EQ(set.at(0).max, -6.0);
    EXPECT_DOUBLE_EQ(set.at(1).min, 12.0);
    EXPECT_DOUBLE_EQ(set.at(1).max, 20.0);
}

TEST(RangeUnion, TheIntersectionIsWalkedInOnePassOverBothSides)
{
    const std::vector<double> aLo{0.0, 10.0};
    const std::vector<double> aHi{4.0, 14.0};
    const std::vector<double> bLo{3.0, 12.0};
    const std::vector<double> bHi{11.0, 20.0};

    RangeUnion set = RangeUnion::of(aLo.data(), aHi.data(), aLo.size());
    set.intersectWith(RangeUnion::of(bLo.data(), bHi.data(), bLo.size()));

    ASSERT_EQ(set.count(), 3u);
    EXPECT_DOUBLE_EQ(set.at(0).min, 3.0);
    EXPECT_DOUBLE_EQ(set.at(0).max, 4.0);
    EXPECT_DOUBLE_EQ(set.at(1).min, 10.0);
    EXPECT_DOUBLE_EQ(set.at(1).max, 11.0);
    EXPECT_DOUBLE_EQ(set.at(2).min, 12.0);
    EXPECT_DOUBLE_EQ(set.at(2).max, 14.0);
}

TEST(RangeUnion, ShiftingCarriesTheWholeSet)
{
    const std::vector<double> lo{-kInfinity, 12.0};
    const std::vector<double> hi{-6.0, kInfinity};

    RangeUnion set = RangeUnion::of(lo.data(), hi.data(), lo.size());
    set.shiftBy(-17.0);

    ASSERT_EQ(set.count(), 2u);
    EXPECT_EQ(set.at(0).min, -kInfinity);
    EXPECT_DOUBLE_EQ(set.at(0).max, -23.0);
    EXPECT_DOUBLE_EQ(set.at(1).min, -5.0);
    EXPECT_EQ(set.at(1).max, kInfinity);
}

TEST(RangeUnion, TheSmallestAdmissibleGainCanLieInTheLowerBranch)
{
    const double mu = 10.0 * std::log10(101.0) - 10.0 * std::log10(2.0);
    ASSERT_NEAR(mu, 17.0329137, 1e-6);

    const std::vector<double> closedLo{-kInfinity, 12.0};
    const std::vector<double> closedHi{-6.0, kInfinity};

    RangeUnion gains = RangeUnion::of(closedLo.data(), closedHi.data(), closedLo.size());

    RangeUnion open = RangeUnion::of(-10.0, kInfinity);

    gains.intersectWith(open);
    gains.shiftBy(-mu);
    gains.intersectWith(20.0 * std::log10(0.01), 20.0 * std::log10(1000.0));

    ASSERT_EQ(gains.count(), 2u);

    EXPECT_DOUBLE_EQ(gains.minimum(), -10.0 - mu);
    EXPECT_DOUBLE_EQ(gains.at(0).max, -6.0 - mu);
    EXPECT_DOUBLE_EQ(gains.at(1).min, 12.0 - mu);

    const double kFeasible = std::pow(10.0, gains.minimum() / 20.0);
    const double kUpperBranch = std::pow(10.0, gains.at(1).min / 20.0);

    EXPECT_DOUBLE_EQ(kUpperBranch / kFeasible, std::pow(10.0, 22.0 / 20.0));
    EXPECT_LT(kFeasible, kUpperBranch);
}
