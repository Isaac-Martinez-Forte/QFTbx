/**
 * @file
 * @brief The GPU base on the card: memory there and back, reservations in
 * stream order, and a kernel that stops when the computation is cancelled.
 */

#include <gtest/gtest.h>

#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "tests/gpu/card.h"
#include "tests/gpu/test_kernels.h"

using namespace qftbx;

TEST(DeviceBase, ValuesGoToTheCardAndComeBack)
{
    QFTBX_REQUIRE_CARD();

    std::vector<double> values(100000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = 0.5 * static_cast<double>(i) - 3.0;
    }

    const std::vector<double> doubled = gputest::doubledOnTheCard(values);
    ASSERT_EQ(doubled.size(), values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        ASSERT_EQ(doubled[i], 2.0 * values[i]) << i;
    }
    EXPECT_TRUE(gputest::doubledOnTheCard({}).empty());
}

TEST(DeviceBase, ReservationsInStreamOrderFollowEachOther)
{
    QFTBX_REQUIRE_CARD();

    EXPECT_TRUE(gputest::reservationsFollowEachOther(1000, 1 << 16));
}

TEST(DeviceBase, AKernelStopsWhenTheComputationIsCancelled)
{
    QFTBX_REQUIRE_CARD();

    CancellationToken token;
    const gputest::SpinOutcome outcome = gputest::spinUntilCancelled(token);
    EXPECT_FALSE(outcome.finished);
    EXPECT_TRUE(outcome.stoppedOnTheFlag);
}
