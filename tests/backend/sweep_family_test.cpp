/**
 * @file
 * @brief The check of the family's poles before a sweep.
 *
 * The combinations are gone through in order: the refusal names the first
 * member and the first whose number of right half-plane poles differs, with
 * their denominator values, and a member whose poles cannot be placed stops
 * the check before a later one that differs, leaving the count unknown. The
 * frequencies of the poles on the imaginary axis sweep a band, and a design
 * frequency strictly inside it is marked with it.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "src/core/common/exception.h"
#include "src/core/math/range.h"
#include "src/core/system/free_form.h"
#include "src/core/system/parameter.h"
#include "src/core/templates/sweep_plan.h"

using namespace qftbx;

namespace {

FreeForm overDenominator(const std::string & denominator, double nominal)
{
    return FreeForm("P", {}, {Parameter("a", Range(-10.0, 10.0), nominal)}, Parameter(1.0), Parameter(0.0), "1",
                    denominator);
}

FreeForm polynomialAtTheEnds()
{
    return overDenominator("s+a+(1-a^2)*sqrt(s)", 1.0);
}

}

TEST(SweepFamily, TheRefusalNamesTheFirstMemberAndTheFirstThatDiffers)
{
    FreeForm plant = overDenominator("s^2-a", 4.0);
    const ParameterGrids grids{{"a", {-1.0, -4.0, 0.5, 2.0}}};
    try {
        planSweep(plant, {0.3}, grids, false);
        FAIL() << "a family that crosses the axis was swept";
    } catch (const InvalidInput & refusal) {
        EXPECT_NE(std::string(refusal.what()).find("0 with the denominator values (-1) and 1 with (0.5)"),
                  std::string::npos) << refusal.what();
    }
}

TEST(SweepFamily, AMemberWhosePolesCannotBePlacedStopsTheCheck)
{
    FreeForm plant = polynomialAtTheEnds();

    const SweepPlan stopped = planCombinations(plant, {0.3}, {{"a", {1.0, 0.0, -1.0}}}, false);
    const FamilyPoles before = checkFamily(plant, stopped);
    EXPECT_EQ(before.firstCount, 0);
    EXPECT_EQ(before.unplaceableAt, 1u);
    EXPECT_FALSE(before.otherCountAt.has_value());
    const SweepPlan unknown = planSweep(plant, {0.3}, {{"a", {1.0, 0.0, -1.0}}}, false);
    EXPECT_FALSE(unknown.familyRhpPoles.has_value());

    const SweepPlan differing = planCombinations(plant, {0.3}, {{"a", {1.0, -1.0, 0.0}}}, false);
    const FamilyPoles after = checkFamily(plant, differing);
    EXPECT_EQ(after.otherCountAt, 1u);
    EXPECT_EQ(after.otherCount, 1);
    EXPECT_FALSE(after.unplaceableAt.has_value());
    EXPECT_THROW(planSweep(plant, {0.3}, {{"a", {1.0, -1.0, 0.0}}}, false), InvalidInput);
}

TEST(SweepFamily, TheBandThePolesOnTheAxisSweepMarksTheFrequenciesInside)
{
    FreeForm plant = overDenominator("s^2+a", 2.0);
    const std::vector<double> frequencies{0.5, 1.2, 3.0};
    const SweepPlan plan = planSweep(plant, frequencies, {{"a", {1.0, 2.25, 4.0}}}, false);

    EXPECT_EQ(plan.familyRhpPoles, 0);
    ASSERT_EQ(plan.resonanceBands.size(), 3u);
    EXPECT_FALSE(plan.resonanceBands[0].has_value());
    ASSERT_TRUE(plan.resonanceBands[1].has_value());
    EXPECT_NEAR((*plan.resonanceBands[1])[0], 1.0, 1e-12);
    EXPECT_NEAR((*plan.resonanceBands[1])[1], 2.0, 1e-12);
    EXPECT_FALSE(plan.resonanceBands[2].has_value());
}
