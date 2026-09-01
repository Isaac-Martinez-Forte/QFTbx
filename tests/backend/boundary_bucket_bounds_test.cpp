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

#include <QMap>
#include <QPointF>
#include <QString>
#include <QVector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/loop_shaping_types.h"

namespace {

//A one-frequency boundary set over a phase window of [phaseStart, 0] with
//phaseCount buckets, holding a single closed boundary point.
BoundaryData * narrowWindow(qreal phaseStart, qint32 phaseCount)
{
    auto * buckets = new QVector<QVector<QVector<QPointF> *> *>();
    auto * row = new QVector<QVector<QPointF> *>();
    for (qint32 i = 0; i < phaseCount; i++) {
        row->append(new QVector<QPointF>());
    }
    //One boundary point, in the first bucket.
    row->at(0)->append(QPointF(phaseStart, 0.0));
    buckets->append(row);

    auto * unionBoundaries = new QVector<QVector<QPointF> *>();
    unionBoundaries->append(new QVector<QPointF>{QPointF(phaseStart, 0.0)});

    auto * boundaries = new QVector<QMap<QString, QVector<QVector<QPointF> *> *> *>();
    boundaries->append(new QMap<QString, QVector<QVector<QPointF> *> *>());

    auto * open = new QVector<bool>{false};
    auto * upper = new QVector<bool>{true};

    return new BoundaryData(boundaries, open, upper, phaseCount,
                            QPointF(phaseStart, 0.0), unionBoundaries, buckets,
                            121, QPointF(-60.0, 60.0));
}

TEST(BoundaryBucketBounds, APhaseOutsideTheNicholsWindowStaysInsideTheBuckets)
{
    //A window of [-180, 0] with a loop point at -300 degrees: the reader used
    //to scale -300 by (phaseCount - 1) / 180 and walk off the end of the
    //bucket row (QVector::at is undefined behaviour out of range, and the
    //box classification reaches the same row with value(), which answers
    //nullptr and is then dereferenced).
    BoundaryData * boundaries = narrowWindow(-180.0, 181);

    BoundaryViolationDetector detector;

    //The verdict itself is not the point here: not reading out of bounds is.
    tools::BoxFlag verdict = detector.classifyPoint(QPointF(-300.0, 10.0), boundaries, 0);
    EXPECT_TRUE(verdict == tools::feasible || verdict == tools::infeasible);

    //The far edge of the window is the last valid bucket, not one past it.
    verdict = detector.classifyPoint(QPointF(-180.0, 10.0), boundaries, 0);
    EXPECT_TRUE(verdict == tools::feasible || verdict == tools::infeasible);

    delete boundaries;
}

TEST(BoundaryBucketBounds, TheFullWindowEdgeIsTheLastBucket)
{
    //The default window: -360 degrees must land on the last bucket of 361,
    //not on bucket 361.
    BoundaryData * boundaries = narrowWindow(-360.0, 361);

    BoundaryViolationDetector detector;

    tools::BoxFlag verdict = detector.classifyPoint(QPointF(-360.0, 10.0), boundaries, 0);
    EXPECT_TRUE(verdict == tools::feasible || verdict == tools::infeasible);

    delete boundaries;
}

} // namespace
