// The phase bucketing of the boundary union is computed in TWO places with
// the same formula: BoundaryUnion1D::bucketIndex builds the buckets and
// BoundaryViolationDetector::phaseBucket reads them back. They must agree on
// the range of valid indices, because the reader indexes the vector the
// writer sized.
//
// The Nichols phase window is free text in the boundaries dialog (defaulted
// to [-360, 0] but not constrained), while every caller normalises a loop
// phase into (-360, 0]. A narrower window than the phase it is asked about
// therefore produces an index past the end of the bucket row.

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

//A one-frequency boundary set over a phase window of [phaseStart, 0] with
//phaseCount buckets, holding a single closed boundary point.
//By value throughout: fifteen lines of heap allocation and a takeOwnership()
//call became five, and there is nothing left for the fixture to free.
BoundaryData narrowWindow(double phaseStart, std::int32_t phaseCount)
{
    //One bucket row of phaseCount buckets, with one boundary point in the
    //first bucket.
    qftbx::UnionBuckets buckets{qftbx::TraceSet(static_cast<std::size_t>(phaseCount))};
    buckets[0][0].push_back(qftbx::NicholsPoint(phaseStart, 0.0));

    return BoundaryData({{}}, {false}, {true}, phaseCount, qftbx::Range(phaseStart, 0.0),
                        {{qftbx::NicholsPoint(phaseStart, 0.0)}}, std::move(buckets),
                        121, qftbx::Range(-60.0, 60.0));
}

TEST(BoundaryBucketBounds, APhaseOutsideTheNicholsWindowStaysInsideTheBuckets)
{
    //A window of [-180, 0] with a loop point at -300 degrees: the reader used
    //to scale -300 by (phaseCount - 1) / 180 and walk off the end of the
    //bucket row (QVector::at is undefined behaviour out of range, and the
    //box classification reaches the same row with value(), which answers
    //nullptr and is then dereferenced).
    const BoundaryData boundaries = narrowWindow(-180.0, 181);

    BoundaryViolationDetector detector;

    //The verdict itself is not the point here: not reading out of bounds is.
    qftbx::BoxFlag verdict = detector.classifyPoint(qftbx::NicholsPoint(-300.0, 10.0), &boundaries, 0);
    EXPECT_TRUE(verdict == qftbx::feasible || verdict == qftbx::infeasible);

    //The far edge of the window is the last valid bucket, not one past it.
    verdict = detector.classifyPoint(qftbx::NicholsPoint(-180.0, 10.0), &boundaries, 0);
    EXPECT_TRUE(verdict == qftbx::feasible || verdict == qftbx::infeasible);
}

TEST(BoundaryBucketBounds, TheFullWindowEdgeIsTheLastBucket)
{
    //The default window: -360 degrees must land on the last bucket of 361,
    //not on bucket 361.
    const BoundaryData boundaries = narrowWindow(-360.0, 361);

    BoundaryViolationDetector detector;

    qftbx::BoxFlag verdict = detector.classifyPoint(qftbx::NicholsPoint(-360.0, 10.0), &boundaries, 0);
    EXPECT_TRUE(verdict == qftbx::feasible || verdict == qftbx::infeasible);
}

//A window whose width is NOT a whole number of degrees, with an open floor
//of one cell at 0 dB on every grid node but one: the columns that hold a
//cell forbid everything under it; the column without one constrains
//nothing. Traced cells only, no stored columns: the rebuild from the traces.
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
    //The reader took the window WIDTH as an integer (its two functions
    //declared the cell count and the span with their types crossed), so a
    //window of 1.5 degrees was scaled as if it were 1: exact on the default
    //360-degree window, wrong on any other.
    //
    //Window [-1.5, 0] with 4 nodes: 3 intervals over 1.5 degrees is a node
    //every 0.5 degrees, so the cell at -1.0 sits on node 1 and its column
    //spans [-1.25, -0.75], and node 2 at -0.5 holds no cell. Truncating the
    //width to 1 would have moved them.
    const BoundaryData boundaries = fractionalWindow(-1.5, 4, -0.5);

    BoundaryViolationDetector detector;

    //The column of a cell: above it feasible, under it not.
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-1.0, 10.0), &boundaries, 0), qftbx::feasible);
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-1.1, -10.0), &boundaries, 0), qftbx::infeasible);

    //The column without a cell finds no crossing.
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.6, -10.0), &boundaries, 0), qftbx::feasible);
}

TEST(BoundaryBucketBounds, ASubDegreeWindowDoesNotDivideByZero)
{
    //Under one degree the truncated width was ZERO: the scale divided by it,
    //the index came out infinite and the clamp sent every phase to the last
    //cell, whatever it was asked.
    const BoundaryData boundaries = fractionalWindow(-0.5, 3, -0.5);

    BoundaryViolationDetector detector;

    //Half the window of 0.5 degrees, with 2 intervals over it: a node every
    //0.25 degrees, so -0.25 is node 1, which holds a cell, and node 0 at
    //-0.5 holds none.
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.25, 10.0), &boundaries, 0), qftbx::feasible);
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.25, -10.0), &boundaries, 0), qftbx::infeasible);
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.45, -10.0), &boundaries, 0), qftbx::feasible);
}

TEST(BoundaryBucketBounds, LoopShapingRefusesAWindowNarrowerThanTheLoopPhase)
{
    //The clamp keeps the read inside the buckets, but a point outside the
    //window would then get the verdict of the edge bucket, which nobody
    //computed. The search says so instead, once and before any algorithm
    //starts (a throw from inside an OpenMP region would end the process).
    const BoundaryData narrow = narrowWindow(-180.0, 181);

    LoopShaping search;

    //The check runs before anything is dereferenced, which is the point of
    //putting it first: the rest of the arguments are never touched.
    EXPECT_THROW(search.run(nullptr, nullptr, nullptr, &narrow, 0.0,
                            qftbx::nt, {}, nullptr, 0),
                 qftbx::ComputationError);

    //That a 360 degree window is ACCEPTED needs no assertion here: every
    //loop-shaping golden test runs over the default [-360, 0] window and
    //would fail at once if this rejected it.
}

} // namespace
