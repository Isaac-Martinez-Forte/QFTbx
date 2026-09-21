/**
 * @file
 * @brief Tests that phase bucket indices stay inside the boundary union.
 *
 * The bucket index is computed by the writer of the union and again by the
 * reader in the violation detector, and both must agree on the valid range.
 * The Nichols phase window is free text in the dialog while loop phases are
 * normalised into (-360, 0], so the cases probe phases outside the window,
 * the far edge landing on the last bucket, windows whose width is not a
 * whole number of degrees or is under one degree, the conservative reading
 * that consults both bracketing nodes, and that loop shaping refuses a
 * window narrower than the loop phase before any algorithm starts.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <string>

#include "src/core/math/point.h"

#include "src/core/math/range.h"

#include "src/core/boundaries/boundary_data.h"
#include "src/core/common/exception.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/loop_shaping.h"
#include "src/core/loopshaping/loop_shaping_types.h"

using namespace qftbx;

namespace {

BoundaryData narrowWindow(double phaseStart, std::int32_t phaseCount)
{
    qftbx::UnionBuckets buckets{qftbx::TraceSet(static_cast<std::size_t>(phaseCount))};
    buckets[0][0].push_back(qftbx::NicholsPoint(phaseStart, 0.0));

    return BoundaryData({{}}, {false}, {true}, phaseCount, qftbx::Range(phaseStart, 0.0),
                        {{qftbx::NicholsPoint(phaseStart, 0.0)}}, std::move(buckets),
                        121, qftbx::Range(-60.0, 60.0));
}

TEST(BoundaryBucketBounds, APhaseOutsideTheNicholsWindowStaysInsideTheBuckets)
{
    const BoundaryData boundaries = narrowWindow(-180.0, 181);

    BoundaryViolationDetector detector;

    qftbx::BoxFlag verdict = detector.classifyPoint(qftbx::NicholsPoint(-300.0, 10.0), &boundaries, 0);
    EXPECT_TRUE(verdict == qftbx::feasible || verdict == qftbx::infeasible);

    verdict = detector.classifyPoint(qftbx::NicholsPoint(-180.0, 10.0), &boundaries, 0);
    EXPECT_TRUE(verdict == qftbx::feasible || verdict == qftbx::infeasible);
}

TEST(BoundaryBucketBounds, TheFullWindowEdgeIsTheLastBucket)
{
    const BoundaryData boundaries = narrowWindow(-360.0, 361);

    BoundaryViolationDetector detector;

    qftbx::BoxFlag verdict = detector.classifyPoint(qftbx::NicholsPoint(-360.0, 10.0), &boundaries, 0);
    EXPECT_TRUE(verdict == qftbx::feasible || verdict == qftbx::infeasible);
}

BoundaryData fractionalWindow(double phaseStart, std::int32_t phaseCount, double skippedPhase)
{
    qftbx::Trace floor;
    const double step = -phaseStart / (phaseCount - 1);
    for (std::int32_t i = 0; i < phaseCount; ++i) {
        const double phase = phaseStart + i * step;
        if (phase != skippedPhase) {
            floor.push_back(qftbx::NicholsPoint(phase, 0.0));
        }
    }
    std::map<std::string, qftbx::TraceSet> specifications{{"Tracking", {floor}}};

    return BoundaryData({std::move(specifications)}, {true}, {true}, phaseCount, qftbx::Range(phaseStart, 0.0),
                        {floor}, {qftbx::TraceSet(static_cast<std::size_t>(phaseCount))},
                        121, qftbx::Range(-60.0, 60.0));
}

TEST(BoundaryBucketBounds, AFractionalPhaseWindowScalesTheBucketsCorrectly)
{
    const BoundaryData boundaries = fractionalWindow(-1.5, 4, -0.5);

    BoundaryViolationDetector detector;

    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-1.0, 10.0), &boundaries, 0), qftbx::feasible);
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-1.1, -10.0), &boundaries, 0), qftbx::infeasible);

    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.6, -10.0), &boundaries, 0), qftbx::feasible);

    BoundaryViolationDetector conservative(true);
    EXPECT_EQ(conservative.classifyPoint(qftbx::NicholsPoint(-0.6, -10.0), &boundaries, 0), qftbx::infeasible);
    EXPECT_EQ(conservative.classifyPoint(qftbx::NicholsPoint(-0.4, -10.0), &boundaries, 0), qftbx::infeasible);
    EXPECT_EQ(conservative.classifyPoint(qftbx::NicholsPoint(-0.5, -10.0), &boundaries, 0), qftbx::feasible);
}

TEST(BoundaryBucketBounds, ASubDegreeWindowDoesNotDivideByZero)
{
    const BoundaryData boundaries = fractionalWindow(-0.5, 3, -0.5);

    BoundaryViolationDetector detector;

    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.25, 10.0), &boundaries, 0), qftbx::feasible);
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.25, -10.0), &boundaries, 0), qftbx::infeasible);
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.45, -10.0), &boundaries, 0), qftbx::feasible);

    BoundaryViolationDetector conservative(true);
    EXPECT_EQ(conservative.classifyPoint(qftbx::NicholsPoint(-0.45, -10.0), &boundaries, 0), qftbx::infeasible);
    EXPECT_EQ(conservative.classifyPoint(qftbx::NicholsPoint(-0.5, -10.0), &boundaries, 0), qftbx::feasible);
}

TEST(BoundaryBucketBounds, LoopShapingRefusesAWindowNarrowerThanTheLoopPhase)
{
    const BoundaryData narrow = narrowWindow(-180.0, 181);

    LoopShaping search;

    EXPECT_THROW(search.run(nullptr, nullptr, nullptr, &narrow, 0.0,
                            qftbx::nt, {}, nullptr, 0),
                 qftbx::ComputationError);

}

}
