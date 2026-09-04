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

#include <string>

#include "src/core/point.h"

#include "src/core/range.h"

#include "src/core/boundaries/boundary_data.h"
#include "src/core/exception.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
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

//A window whose width is NOT a whole number of degrees, with one boundary
//point in a chosen bucket and nothing anywhere else. An open boundary whose
//allowed side is up gives a parity rule the bucket can be read through: one
//boundary point below the query means feasible, an empty bucket means
//infeasible.
BoundaryData fractionalWindow(double phaseStart, std::int32_t phaseCount, std::size_t pointBucket)
{
    qftbx::UnionBuckets buckets{qftbx::TraceSet(static_cast<std::size_t>(phaseCount))};
    buckets[0][pointBucket].push_back(qftbx::NicholsPoint(phaseStart, 0.0));

    return BoundaryData({{}}, {true}, {true}, phaseCount, qftbx::Range(phaseStart, 0.0),
                        {{qftbx::NicholsPoint(phaseStart, 0.0)}}, std::move(buckets),
                        121, qftbx::Range(-60.0, 60.0));
}

TEST(BoundaryBucketBounds, AFractionalPhaseWindowScalesTheBucketsCorrectly)
{
    //The reader took the window WIDTH as an integer (its two functions
    //declared the cell count and the span with their types crossed), so a
    //window of 1.5 degrees was scaled as if it were 1: exact on the default
    //360-degree window, wrong on any other.
    //
    //Window [-1.5, 0] with 4 cells: 3 intervals over 1.5 degrees is 2 cells
    //per degree, so -1.0 degrees belongs to cell 2. Truncating the width to
    //1 gives 3 cells per degree and sends it to cell 3.
    const BoundaryData boundaries = fractionalWindow(-1.5, 4, 2);

    BoundaryViolationDetector detector;

    //Cell 2 holds the boundary point, one below the query: feasible.
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-1.0, 10.0), &boundaries, 0), qftbx::feasible);

    //And a phase that belongs elsewhere still finds its own cell empty.
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.25, 10.0), &boundaries, 0), qftbx::infeasible);
}

TEST(BoundaryBucketBounds, ASubDegreeWindowDoesNotDivideByZero)
{
    //Under one degree the truncated width was ZERO: the scale divided by it,
    //the index came out infinite and the clamp sent every phase to the last
    //cell, whatever it was asked.
    const BoundaryData boundaries = fractionalWindow(-0.5, 3, 1);

    BoundaryViolationDetector detector;

    //Half the window of 0.5 degrees, with 2 cells over it: 4 cells per
    //degree, so -0.25 belongs to cell 1, which holds the point.
    EXPECT_EQ(detector.classifyPoint(qftbx::NicholsPoint(-0.25, 10.0), &boundaries, 0), qftbx::feasible);
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
