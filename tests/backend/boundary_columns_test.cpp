/**
 * @file
 * @brief Tests of the allowed magnitude intervals per phase column.
 *
 * Hand-made sheets on a 1 degree by 0.5 dB grid whose truth is known are
 * cut at 0 dB and the columns and the detector's verdicts are read off them:
 * an open boundary allowing above, one allowing below, a closed curve, the
 * corridor between an open floor and a closed curve, a fragment a cell thick
 * that is a band and not a crossing, a pocket inside a curve, a node that is
 * not a number, and the crossing interpolated between nodes. The rebuild
 * from traced curves, for files that predate the columns, is checked on the
 * same shapes, including the thin fragment of QFT toolbox example 2 at 100
 * rad/s. Intersection across different phase grids must be refused.
 */

#include <gtest/gtest.h>

#include "src/core/common/exception.h"

#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <string>

#include "src/core/boundaries/boundary_columns.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/math/range.h"

using namespace qftbx;

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

constexpr std::int32_t kPhases = 361;
constexpr std::int32_t kMagnitudes = 241;
const Range kPhaseRange(-360.0, 0.0);
const Range kMagnitudeRange(-60.0, 60.0);

BoundaryColumns sheetOf(const std::function<double(double, double)> & d)
{
    const auto cell = [&d](std::int32_t phase, std::int32_t magnitude) {
        return d(-360.0 + phase, -60.0 + 0.5 * magnitude);
    };
    return BoundaryColumns::fromSheet(cell, 0.0, kPhases, kPhaseRange, kMagnitudes, kMagnitudeRange);
}

double above10(double, double m) { return 10.0 - m; }
double below10(double, double m) { return m - 10.0; }
double aboveMinus20(double, double m) { return -20.0 - m; }

double rectangle(double phase, double m)
{
    return (std::fabs(phase + 180.0) <= 20.0 && std::fabs(m) <= 15.0) ? 1.0 : -1.0;
}

double corridor(double phase, double m) { return std::max(aboveMinus20(phase, m), rectangle(phase, m)); }

double fragment(double phase, double m)
{
    if (std::fabs(phase + 180.0) <= 2.0 && m == -9.0) {
        return 0.0;
    }
    return -43.0 - m;
}

double pocket(double phase, double m)
{
    const bool outer = std::fabs(phase + 180.0) <= 30.0 && std::fabs(m) <= 20.0;
    const bool inner = std::fabs(phase + 180.0) <= 10.0 && std::fabs(m) <= 5.0;
    return (outer && !inner) ? 1.0 : -1.0;
}

BoundaryData dataOf(std::map<std::string, BoundaryColumns> columns)
{
    return BoundaryData({{}}, {true}, {true}, kPhases, kPhaseRange, {{}}, {{}},
                        kMagnitudes, kMagnitudeRange, ColumnSet{std::move(columns)});
}

NicholsBox boxOf(double phaseLo, double phaseHi, double magLo, double magHi)
{
    return {Interval(magLo, magHi), Interval(phaseLo, phaseHi)};
}

Trace curve(double (*f)(double))
{
    Trace trace;
    for (int i = 0; i < kPhases; ++i) {
        const double phase = -360.0 + i;
        trace.push_back(NicholsPoint(phase, f(phase)));
    }
    return trace;
}

double flat10(double) { return 10.0; }
double flatMinus20(double) { return -20.0; }
double flatMinus43(double) { return -43.0; }

Trace closedRectangle(double centrePhase, double centreMag, double halfWidth, double halfHeight)
{
    Trace trace;
    for (double phase = centrePhase - halfWidth; phase <= centrePhase + halfWidth; phase += 1.0) {
        trace.push_back(NicholsPoint(phase, centreMag + halfHeight));
        trace.push_back(NicholsPoint(phase, centreMag - halfHeight));
    }
    for (double mag = centreMag - halfHeight; mag <= centreMag + halfHeight; mag += 0.5) {
        trace.push_back(NicholsPoint(centrePhase - halfWidth, mag));
        trace.push_back(NicholsPoint(centrePhase + halfWidth, mag));
    }
    return trace;
}

BoundaryColumns fromTraces(const std::string & specification, TraceSet traces)
{
    return BoundaryColumns::fromTraces(specification, traces, kPhases, kPhaseRange, kMagnitudes, kMagnitudeRange);
}

}

TEST(BoundaryColumns, OpenBoundaryAllowedAbove)
{
    const BoundaryData data = dataOf({{"Tracking", sheetOf(above10)}});
    BoundaryViolationDetector detector;

    const BoundaryColumns::Intervals column = data.columns(0).intervals(data.columns(0).columnOf(-100.0));
    ASSERT_EQ(column.count, 1);
    EXPECT_DOUBLE_EQ(column.lo[0], 10.0);
    EXPECT_EQ(column.hi[0], kInf);

    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-100.0, 20.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-100.0, 10.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-100.0, 9.9), &data, 0), infeasible);

    EXPECT_EQ(detector.classifyBox(boxOf(-200.0, -100.0, 12.0, 30.0), &data, 0).flag(), feasible);
    EXPECT_EQ(detector.classifyBox(boxOf(-200.0, -100.0, -30.0, 5.0), &data, 0).flag(), infeasible);
    const BoxClassification straddling = detector.classifyBox(boxOf(-200.0, -100.0, 5.0, 15.0), &data, 0);
    EXPECT_EQ(straddling.flag(), ambiguous);
    EXPECT_DOUBLE_EQ(straddling.extremes()[0], 10.0);
    EXPECT_DOUBLE_EQ(straddling.extremes()[1], 10.0);
    EXPECT_TRUE(straddling.isBottomLeftForbidden());
    EXPECT_FALSE(straddling.isTopRightForbidden());
}

TEST(BoundaryColumns, OpenBoundaryAllowedBelow)
{
    const BoundaryData data = dataOf({{"ControlEffort", sheetOf(below10)}});
    BoundaryViolationDetector detector;

    const BoundaryColumns::Intervals column = data.columns(0).intervals(100);
    ASSERT_EQ(column.count, 1);
    EXPECT_EQ(column.lo[0], -kInf);
    EXPECT_DOUBLE_EQ(column.hi[0], 10.0);

    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-100.0, 20.0), &data, 0), infeasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-100.0, 9.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyBox(boxOf(-300.0, -50.0, -30.0, 5.0), &data, 0).flag(), feasible);
    EXPECT_EQ(detector.classifyBox(boxOf(-300.0, -50.0, 12.0, 30.0), &data, 0).flag(), infeasible);
}

TEST(BoundaryColumns, ClosedBoundaryForbidsItsInside)
{
    const BoundaryData data = dataOf({{"Stability", sheetOf(rectangle)}});
    BoundaryViolationDetector detector;

    const BoundaryColumns::Intervals column = data.columns(0).intervals(180);
    ASSERT_EQ(column.count, 2);
    EXPECT_EQ(column.lo[0], -kInf);
    EXPECT_DOUBLE_EQ(column.hi[0], -15.25);
    EXPECT_DOUBLE_EQ(column.lo[1], 15.25);
    EXPECT_EQ(column.hi[1], kInf);

    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 0.0), &data, 0), infeasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 20.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, -20.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-250.0, 0.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-200.0, 0.0), &data, 0), infeasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-201.0, 0.0), &data, 0), feasible);

    EXPECT_EQ(detector.classifyBox(boxOf(-190.0, -170.0, -10.0, 10.0), &data, 0).flag(), infeasible);
    EXPECT_EQ(detector.classifyBox(boxOf(-190.0, -170.0, 20.0, 30.0), &data, 0).flag(), feasible);
    EXPECT_EQ(detector.classifyBox(boxOf(-190.0, -170.0, 10.0, 20.0), &data, 0).flag(), ambiguous);
    EXPECT_EQ(detector.classifyBox(boxOf(-210.0, -190.0, -5.0, 5.0), &data, 0).flag(), ambiguous);
}

TEST(BoundaryColumns, CorridorBetweenAnOpenFloorAndAClosedCurve)
{
    const BoundaryData separate = dataOf({{"Tracking", sheetOf(aboveMinus20)}, {"Stability", sheetOf(rectangle)}});
    const BoundaryData joint = dataOf({{"Tracking", sheetOf(corridor)}});
    BoundaryViolationDetector detector;

    for (const BoundaryData * data : {&separate, &joint}) {
        EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, -25.0), data, 0), infeasible);
        EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, -18.0), data, 0), feasible);
        EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 0.0), data, 0), infeasible);
        EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 20.0), data, 0), feasible);

        const BoxClassification inside = detector.classifyBox(boxOf(-185.0, -175.0, -5.0, 5.0), data, 0);
        EXPECT_EQ(inside.flag(), infeasible);
        EXPECT_TRUE(inside.isBottomLeftForbidden());

        EXPECT_EQ(detector.classifyBox(boxOf(-185.0, -175.0, -19.0, -16.0), data, 0).flag(), feasible);

        const BoxClassification tall = detector.classifyBox(boxOf(-185.0, -175.0, -30.0, 30.0), data, 0);
        EXPECT_EQ(tall.flag(), ambiguous);
        EXPECT_DOUBLE_EQ(tall.extremes()[0], -20.0);
        EXPECT_DOUBLE_EQ(tall.extremes()[1], 15.25);
    }
}

TEST(BoundaryColumns, FragmentACellThickIsABandNotACrossing)
{
    const BoundaryData data = dataOf({{"Tracking", sheetOf(fragment)}});
    const BoundaryColumns & columns = data.columns(0);

    const BoundaryColumns::Intervals column = columns.intervals(180);
    ASSERT_EQ(column.count, 2);
    EXPECT_DOUBLE_EQ(column.lo[0], -43.0);
    EXPECT_DOUBLE_EQ(column.hi[0], -9.0);
    EXPECT_DOUBLE_EQ(column.lo[1], -9.0);
    EXPECT_EQ(column.hi[1], kInf);
    EXPECT_FALSE(columns.allows(180, -50.0));
    EXPECT_TRUE(columns.allows(180, -20.0));
    EXPECT_TRUE(columns.allows(180, 0.0));

    const BoundaryColumns::Intervals beside = columns.intervals(170);
    ASSERT_EQ(beside.count, 1);
    EXPECT_DOUBLE_EQ(beside.lo[0], -43.0);
}

TEST(BoundaryColumns, PocketInsideACurveIsAllowed)
{
    const BoundaryData data = dataOf({{"Tracking", sheetOf(pocket)}});
    BoundaryViolationDetector detector;

    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 0.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 10.0), &data, 0), infeasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, 30.0), &data, 0), feasible);
    EXPECT_EQ(detector.classifyPoint(NicholsPoint(-180.0, -30.0), &data, 0), feasible);
    EXPECT_EQ(data.columns(0).intervals(180).count, 3);
}

TEST(BoundaryColumns, ANodeThatIsNotANumberViolates)
{
    const BoundaryData data = dataOf({{"Tracking", sheetOf([](double phase, double m) {
        return (phase == -180.0 && m == 0.0) ? std::numeric_limits<double>::quiet_NaN() : -1.0;
    })}});
    const BoundaryColumns & columns = data.columns(0);

    EXPECT_FALSE(columns.allows(180, 0.0));
    EXPECT_TRUE(columns.allows(180, 1.0));
    EXPECT_TRUE(columns.allows(180, -1.0));
    EXPECT_EQ(columns.intervals(180).count, 2);
    EXPECT_EQ(columns.intervals(179).count, 1);
}

TEST(BoundaryColumns, CrossingIsInterpolatedBetweenTheNodes)
{
    const BoundaryData data = dataOf({{"Tracking", sheetOf([](double, double m) { return 2.0 * (10.2 - m); })}});
    const BoundaryColumns::Intervals column = data.columns(0).intervals(100);
    ASSERT_EQ(column.count, 1);
    EXPECT_NEAR(column.lo[0], 10.2, 1e-12);
}

TEST(BoundaryColumns, IntersectionAndEverythingAllowed)
{
    BoundaryColumns all(kPhases, kPhaseRange);
    EXPECT_TRUE(all.allows(233, 0.0));
    EXPECT_TRUE(all.allows(0, -1000.0));

    all.intersectWith(sheetOf(above10));
    all.intersectWith(sheetOf(rectangle));
    EXPECT_FALSE(all.allows(180, 5.0));
    EXPECT_FALSE(all.allows(180, 12.0));
    EXPECT_TRUE(all.allows(180, 20.0));
    EXPECT_TRUE(all.allows(100, 12.0));
    EXPECT_FALSE(all.allows(100, 5.0));

    EXPECT_EQ(all, all);
    EXPECT_NE(all, sheetOf(above10));
}

TEST(BoundaryColumns, PhasesOutsideTheWindowFallInTheEndColumns)
{
    const BoundaryColumns columns(kPhases, kPhaseRange);
    EXPECT_EQ(columns.columnOf(-400.0), 0);
    EXPECT_EQ(columns.columnOf(-360.0), 0);
    EXPECT_EQ(columns.columnOf(-127.4), 233);
    EXPECT_EQ(columns.columnOf(-126.6), 233);
    EXPECT_EQ(columns.columnOf(0.0), 360);
    EXPECT_EQ(columns.columnOf(30.0), 360);
    EXPECT_DOUBLE_EQ(columns.phaseOf(233), -127.0);
}

TEST(BoundaryColumns, RebuildFromTracesFollowsTheLabels)
{
    const BoundaryColumns above = fromTraces("Tracking", {curve(flat10)});
    EXPECT_TRUE(above.allows(260, 10.0));
    EXPECT_FALSE(above.allows(260, 9.9));
    EXPECT_TRUE(above.allows(260, 50.0));

    const BoundaryColumns below = fromTraces("ControlEffort", {curve(flat10)});
    EXPECT_TRUE(below.allows(260, 9.0));
    EXPECT_FALSE(below.allows(260, 10.1));

    const BoundaryColumns closed = fromTraces("Stability", {closedRectangle(-180.0, 0.0, 20.0, 15.0)});
    EXPECT_FALSE(closed.allows(180, 0.0));
    EXPECT_TRUE(closed.allows(180, 20.0));
    EXPECT_TRUE(closed.allows(180, -20.0));
    EXPECT_FALSE(closed.allows(160, 0.0));
    EXPECT_TRUE(closed.allows(159, 0.0));

    BoundaryColumns corridor = fromTraces("Tracking", {curve(flatMinus20)});
    corridor.intersectWith(closed);
    EXPECT_FALSE(corridor.allows(180, -25.0));
    EXPECT_TRUE(corridor.allows(180, -18.0));
    EXPECT_FALSE(corridor.allows(180, 0.0));
    EXPECT_TRUE(corridor.allows(180, 20.0));

    const BoundaryColumns pocketed = fromTraces("Tracking", {closedRectangle(-180.0, 0.0, 30.0, 20.0),
                                                             closedRectangle(-180.0, 0.0, 10.0, 5.0)});
    EXPECT_TRUE(pocketed.allows(180, 0.0));
    EXPECT_FALSE(pocketed.allows(180, 10.0));
    EXPECT_TRUE(pocketed.allows(180, 30.0));
}

TEST(BoundaryColumns, RebuildFromTracesTreatsAThinFragmentAsABand)
{
    Trace fragment;
    for (double phase = -182.0; phase <= -178.0; phase += 1.0) {
        fragment.push_back(NicholsPoint(phase, -9.0));
    }
    const BoundaryColumns columns = fromTraces("Tracking", {curve(flatMinus43), fragment});

    EXPECT_FALSE(columns.allows(180, -50.0));
    EXPECT_TRUE(columns.allows(180, -20.0));
    EXPECT_TRUE(columns.allows(180, 0.0));
    EXPECT_FALSE(columns.allows(170, -50.0));
    EXPECT_TRUE(columns.allows(170, -20.0));
}

TEST(BoundaryColumns, DerivedLabelsFollowShapeAndFamily)
{
    const TraceSet open{curve(flat10)};
    const TraceSet closed{closedRectangle(-180.0, 0.0, 20.0, 15.0)};

    EXPECT_EQ(BoundaryColumns::deriveLabels("Tracking", open, kPhaseRange, kPhases), TraceLabels{false});
    EXPECT_EQ(BoundaryColumns::deriveLabels("OutputDisturbance", open, kPhaseRange, kPhases), TraceLabels{false});
    EXPECT_EQ(BoundaryColumns::deriveLabels("Stability", open, kPhaseRange, kPhases), TraceLabels{true});
    EXPECT_EQ(BoundaryColumns::deriveLabels("ControlEffort", open, kPhaseRange, kPhases), TraceLabels{true});
    EXPECT_EQ(BoundaryColumns::deriveLabels("Stability", closed, kPhaseRange, kPhases), TraceLabels{false});
    EXPECT_EQ(BoundaryColumns::deriveLabels("Tracking", closed, kPhaseRange, kPhases), TraceLabels{false});
}

TEST(BoundaryColumnsGrid, IntersectionRefusesADifferentPhaseGrid)
{
    qftbx::BoundaryColumns fine(361, qftbx::Range(-360.0, 0.0));
    qftbx::BoundaryColumns coarse(181, qftbx::Range(-360.0, 0.0));
    qftbx::BoundaryColumns shifted(361, qftbx::Range(-350.0, 10.0));
    qftbx::BoundaryColumns same(361, qftbx::Range(-360.0, 0.0));

    EXPECT_THROW(fine.intersectWith(coarse), qftbx::InvalidInput);
    EXPECT_THROW(fine.intersectWith(shifted), qftbx::InvalidInput);
    EXPECT_NO_THROW(fine.intersectWith(same));
}

TEST(BoundaryColumnsGrid, TheColumnsCoveringAPhaseBracketIt)
{
    qftbx::BoundaryColumns columns(361, qftbx::Range(-360.0, 0.0));

    EXPECT_EQ(columns.firstColumnCovering(-126.0), 234);
    EXPECT_EQ(columns.lastColumnCovering(-126.0), 234);
    EXPECT_EQ(columns.columnOf(-126.0), 234);

    EXPECT_EQ(columns.firstColumnCovering(-126.4983), 233);
    EXPECT_EQ(columns.lastColumnCovering(-126.4983), 234);
    EXPECT_EQ(columns.columnOf(-126.4983), 234);

    EXPECT_EQ(columns.firstColumnCovering(-400.0), 0);
    EXPECT_EQ(columns.lastColumnCovering(-400.0), 0);
    EXPECT_EQ(columns.firstColumnCovering(40.0), 360);
    EXPECT_EQ(columns.lastColumnCovering(40.0), 360);
}
