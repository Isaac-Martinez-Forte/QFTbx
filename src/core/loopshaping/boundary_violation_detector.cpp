#include <cstdint>
#include "src/core/loopshaping/boundary_violation_detector.h"

using namespace tools;
using namespace cxsc;

//Reader of the phase bucketing that BoundaryUnion1D::bucketIndex writes.
//Both must agree on the range of valid indices, because this indexes the
//vector the other one sized: the writer clamps to its last bucket and this
//one did not, so a phase beyond the Nichols window walked off the end of the
//row (at() out of range is undefined behaviour, and the box classification
//reaches the same row through value(), which answers nullptr and was then
//dereferenced). The window is free text in the boundaries dialog while every
//caller normalises phase into (-360, 0], so a window narrower than 360
//degrees was enough to reach it.
std::int32_t BoundaryViolationDetector::phaseBucket(double phaseDegrees, std::int32_t bucketCount,
                                                     double phaseSpanDegrees)
{
    //Cells per degree, times the distance from the window's edge. The cast
    //that used to sit on bucketCount was a leftover of the crossed types.
    double res = std::abs(phaseDegrees) * (bucketCount / phaseSpanDegrees);
    if(res<0) res=0;
    if(res>bucketCount) res=bucketCount;
    return static_cast<std::int32_t>(res);
}

BoxFlag BoundaryViolationDetector::pointVerdict(qftbx::NicholsPoint point,
                                                const qftbx::TraceSet & buckets,
                                                std::int32_t bucketCount,
                                                bool above, double phaseSpanDegrees) {
    std::int32_t crossingsAbove = 0;
    const qftbx::Trace & bucket =
            buckets.at(static_cast<std::size_t>(phaseBucket(point.phase, bucketCount, phaseSpanDegrees)));

    for (const qftbx::NicholsPoint & bucketPoint : bucket) {
        if (point.magnitude > bucketPoint.magnitude){
            crossingsAbove++;
        }
    }

    //Parity test. 'above' comes from the union metadata and means the
    //ALLOWED side is above. An even number of boundary layers below the
    //point leaves it UNDER the union, an odd number over it. The
    //historical open-boundary branch had the two verdicts swapped, so every
    //open boundary (tracking, disturbance rejection) accepted exactly the
    //loops that violated it and rejected the compliant ones; once fixed,
    //the open and the closed branch computed the same thing, and there is
    //one test now.
    const bool underTheUnion = crossingsAbove % 2 == 0;
    const bool violation = underTheUnion ? above : !above;

    return violation ? infeasible : feasible;
}

//Feasibility of a Nichols box against the boundary union at one design
//frequency (Tharewal 2005, sec. 3.3.4): feasible when the box lies
//entirely on the allowed side, infeasible when entirely on the forbidden
//side, ambiguous when boundary points fall inside it. The returned
//minimums/maximums are B_min and B_max, the extreme boundary magnitudes
//over the box's PHASE interval (Tharewal 2005, fig. 5.1), in dB/degrees,
//which drive the gain cutting, plus the boundary's phase extremes over
//the same span, which drive the phase cutting of algorithm MC. The
//historical version computed B_min and B_max only from the boundary
//points INSIDE the box: when the boundary left the box within its phase
//span the cut could remove feasible gains.
BoxClassification BoundaryViolationDetector::classifyBox(cinterval box, const BoundaryData *boundaries, std::size_t frequencyIndex) {

    const qftbx::TraceSet & buckets =
            boundaries->unionBuckets().at(frequencyIndex);
    const std::int32_t bucketCount = boundaries->phaseCount() - 1;
    const bool above = boundaries->upperFlags().at(frequencyIndex);

    double minPhaseBound = std::numeric_limits<double>::max(), maxPhaseBound = std::numeric_limits<double>::lowest(),
            minMagBound = std::numeric_limits<double>::max(), maxMagBound = std::numeric_limits<double>::lowest();

    bool ambiguousVerdict = false;

    const double phaseSpanDegrees = boundaries->phaseRange().width();

    //Degrees per bucket of the phase-bucketed union (the historical
    //formula was inverted, which only worked on the standard 1-degree grid).
    double step = phaseSpanDegrees / bucketCount;


    double minPhase = _double(InfIm(box)), maxPhase = _double(SupIm(box)), minMag = _double(InfRe(box)), maxMag = _double(SupRe(box));

    for (double f = minPhase; f <= maxPhase + step; f += step) {

        //at(), not the old value(): out of range that returned nullptr and
        //was dereferenced. The clamp inside phaseBucket keeps the index in
        //range, and at() would now say so loudly if it ever did not.
        for (const qftbx::NicholsPoint & boundaryPoint :
             buckets.at(static_cast<std::size_t>(phaseBucket(std::min(f, maxPhase), bucketCount, phaseSpanDegrees)))) {

            //Only boundary points within the box's phase span take part.
            if (boundaryPoint.phase < minPhase || boundaryPoint.phase > maxPhase) {
                continue;
            }

            //B_min / B_max over the phase interval, regardless of the
            //box's magnitude range.
            if (boundaryPoint.phase > maxPhaseBound) {
                maxPhaseBound = boundaryPoint.phase;
            }

            if (boundaryPoint.phase < minPhaseBound) {
                minPhaseBound = boundaryPoint.phase;
            }

            if (boundaryPoint.magnitude > maxMagBound) {
                maxMagBound = boundaryPoint.magnitude;
            }

            if (boundaryPoint.magnitude < minMagBound) {
                minMagBound = boundaryPoint.magnitude;
            }

            //A boundary point inside the box makes it ambiguous.
            if (boundaryPoint.magnitude >= minMag && boundaryPoint.magnitude <= maxMag) {
                ambiguousVerdict = true;
            }
        }
    }



    BoxClassification classification;

    classification.setExtremes({minMagBound, maxMagBound, minPhaseBound, maxPhaseBound});

    //Corner classifications for the cutting strips: every boundary point
    //whose phase lies in the box's span was scanned above, so the box
    //regions below/left of (B_min, phase_min) and above/right of
    //(B_max, phase_max) are boundary-free and uniformly classified by the
    //corner they contain. The bottom-left corner drives the gain cutting
    //(NT/NK/MC/thesis-MC, bottom and left strips); the top-right corner
    //drives the top and right strips (MC/thesis-MC).
    BoxFlag f = pointVerdict(qftbx::NicholsPoint(minPhase, minMag), buckets, bucketCount,
                             above, phaseSpanDegrees);
    classification.setBottomLeftForbidden(f == infeasible);

    BoxFlag f2 = pointVerdict(qftbx::NicholsPoint(maxPhase, maxMag), buckets, bucketCount,
                              above, phaseSpanDegrees);
    classification.setTopRightForbidden(f2 == infeasible);

    if (ambiguousVerdict) {
        classification.setFlag(ambiguous);
    } else {
        classification.setFlag(f);
    }

    return classification;
}

//Classification of a single Nichols point (phase in degrees, magnitude in
//dB) against the boundary union at one design frequency, with the same
//parity test the box classification uses. It certifies the zone gates of
//the gain cutting and splitting (Tharewal 2005, ch. 5).
tools::BoxFlag BoundaryViolationDetector::classifyPoint(qftbx::NicholsPoint point, const BoundaryData * boundaries, std::size_t frequencyIndex) {

    const qftbx::TraceSet & buckets =
            boundaries->unionBuckets().at(frequencyIndex);
    const std::int32_t bucketCount = boundaries->phaseCount() - 1;
    const bool above = boundaries->upperFlags().at(frequencyIndex);
    const double phaseSpanDegrees = boundaries->phaseRange().width();

    return pointVerdict(point, buckets, bucketCount, above, phaseSpanDegrees);
}
