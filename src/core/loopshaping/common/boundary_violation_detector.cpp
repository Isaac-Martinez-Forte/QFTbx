#include <cstdint>
#include <limits>

#include "src/core/loopshaping/common/boundary_violation_detector.h"

namespace qftbx {

//Feasibility of a Nichols box against the boundaries of one design frequency
//(Tharewal 2005, sec. 3.3.4): feasible when the box lies entirely on the
//allowed side at every phase column it spans, infeasible when entirely on
//the forbidden side at every column, ambiguous otherwise - a boundary
//crossing inside the box, or columns that disagree. The verdict is read off
//the allowed intervals of the columns (BoundaryColumns); every specification
//is in them with its own semantics, so an open boundary running under a
//closed one, or a corridor between the two, classifies as the
//specifications say. The parity count over the union's bucket that used to
//stand here called the inside of a closed curve allowed whenever the union
//had dropped the curve below it.
//
//The extremes returned with the verdict are B_min and B_max, the lowest and
//highest boundary crossing over the box's PHASE interval regardless of its
//magnitude (Tharewal 2005, fig. 5.1), and the boundary's phase extremes over
//the same span; they drive the gain and phase cutting of the algorithms,
//which read them as the limits of a strip of uniform state: below B_min the
//whole span is forbidden (the cuts C_g- and QS raise the gain to it) or
//allowed (the thesis's feasible bottom strip), as the bottom-left corner
//says; above B_max likewise, as the top-right corner says. On a
//single-valued open boundary that is exactly the minimum and the maximum of
//the curve over the span. On boundaries in general the strip is taken over
//every column of the span - the lowest end of the bottom interval of each
//column when every column is forbidden at the bottom, the lowest top of
//the bottom interval when every column is allowed there, and their mirror
//images at the top - and is infinite when the columns do not agree, so no
//cut applies: a closed curve alone cuts no gain, which is what its
//geometry says. The corner verdicts certify the cutting strips: the
//bottom-left corner the bottom and left strips, the top-right corner the
//top and right ones.
BoxClassification BoundaryViolationDetector::classifyBox(NicholsBox box, const BoundaryData *boundaries, std::size_t frequencyIndex) {
    ++m_classifications;

    constexpr double kInfinity = std::numeric_limits<double>::infinity();

    const BoundaryColumns & columns = boundaries->columns(frequencyIndex);

    const double minPhase = box.phaseDegrees.lower(), maxPhase = box.phaseDegrees.upper();
    const double minMag = box.magnitudeDb.lower(), maxMag = box.magnitudeDb.upper();

    //The columns the box's phase span covers; the end columns take what
    //falls outside the window.
    const std::int32_t first = columns.columnOf(minPhase);
    const std::int32_t last = columns.columnOf(maxPhase);

    double minPhaseBound = std::numeric_limits<double>::max(), maxPhaseBound = std::numeric_limits<double>::lowest();

    //The strips of uniform state under and over the span (see above).
    double forbiddenBelow = kInfinity, allowedBelow = kInfinity;
    double forbiddenAbove = -kInfinity, allowedAbove = -kInfinity;
    bool everyBottomForbidden = true, everyBottomAllowed = true;
    bool everyTopForbidden = true, everyTopAllowed = true;

    bool anyAllowed = false, anyForbidden = false, ambiguousVerdict = false;

    for (std::int32_t c = first; c <= last; ++c) {
        const BoundaryColumns::Intervals spans = columns.intervals(c);
        const double phase = columns.phaseOf(c);

        if (spans.count == 0) {
            //Nothing allowed here: forbidden at every magnitude.
            everyBottomAllowed = false;
            everyTopAllowed = false;
        } else {
            if (spans.lo[0] > -kInfinity) {
                everyBottomAllowed = false;
                if (spans.lo[0] < forbiddenBelow) forbiddenBelow = spans.lo[0];
            } else {
                everyBottomForbidden = false;
                if (spans.hi[0] < allowedBelow) allowedBelow = spans.hi[0];
            }
            const std::int32_t top = spans.count - 1;
            if (spans.hi[top] < kInfinity) {
                everyTopAllowed = false;
                if (spans.hi[top] > forbiddenAbove) forbiddenAbove = spans.hi[top];
            } else {
                everyTopForbidden = false;
                if (spans.lo[top] > allowedAbove) allowedAbove = spans.lo[top];
            }
        }

        //The column's verdict on [minMag, maxMag]: inside one allowed
        //interval, inside one forbidden gap, or across an end.
        bool allowed = false, forbidden = false;
        double previousHi = -kInfinity;
        bool decided = false;

        for (std::int32_t i = 0; i < spans.count; ++i) {
            const double lo = spans.lo[i];
            const double hi = spans.hi[i];

            //Finite ends are boundary crossings: the phase extremes of the
            //boundary over the span, and an end inside the box makes it
            //ambiguous.
            if (lo > -kInfinity) {
                if (phase < minPhaseBound) minPhaseBound = phase;
                if (phase > maxPhaseBound) maxPhaseBound = phase;
                if (lo >= minMag && lo <= maxMag) ambiguousVerdict = true;
            }
            if (hi < kInfinity) {
                if (phase < minPhaseBound) minPhaseBound = phase;
                if (phase > maxPhaseBound) maxPhaseBound = phase;
                if (hi >= minMag && hi <= maxMag) ambiguousVerdict = true;
            }

            if (!decided) {
                if (maxMag < lo) {
                    //Wholly in the gap below this interval.
                    forbidden = minMag > previousHi;
                    decided = true;
                } else if (minMag >= lo && maxMag <= hi) {
                    allowed = true;
                    decided = true;
                } else if (minMag <= hi) {
                    //Straddles an end of this interval.
                    decided = true;
                }
            }
            previousHi = hi;
        }

        if (!decided) {
            //Above the last interval, or a column with no allowed interval.
            forbidden = minMag > previousHi;
        }

        anyAllowed = anyAllowed || allowed;
        anyForbidden = anyForbidden || forbidden;
        if (!allowed && !forbidden) {
            ambiguousVerdict = true;
        }
    }

    BoxClassification classification;

    const bool bottomLeftForbidden = !columns.allows(first, minMag);
    const bool topRightForbidden = !columns.allows(last, maxMag);
    classification.setBottomLeftForbidden(bottomLeftForbidden);
    classification.setTopRightForbidden(topRightForbidden);

    //B_min: the top of the strip under the span whose state the bottom-left
    //corner has; B_max: the bottom of the strip over it whose state the
    //top-right corner has. Infinite when the columns do not share it.
    const double minMagBound = bottomLeftForbidden ? (everyBottomForbidden ? forbiddenBelow : -kInfinity)
                                                   : (everyBottomAllowed ? allowedBelow : -kInfinity);
    const double maxMagBound = topRightForbidden ? (everyTopForbidden ? forbiddenAbove : kInfinity)
                                                 : (everyTopAllowed ? allowedAbove : kInfinity);

    classification.setExtremes({minMagBound, maxMagBound, minPhaseBound, maxPhaseBound});

    if (ambiguousVerdict || (anyAllowed && anyForbidden)) {
        classification.setFlag(ambiguous);
        ++m_ambiguous;
    } else if (anyAllowed) {
        classification.setFlag(feasible);
        ++m_feasible;
    } else {
        classification.setFlag(infeasible);
        ++m_infeasible;
    }

    return classification;
}

//Classification of a single Nichols point (phase in degrees, magnitude in
//dB) against the boundaries of one design frequency: the allowed intervals
//of its phase column. It certifies the zone gates of the gain cutting and
//splitting (Tharewal 2005, ch. 5) and the corner a terminating box returns.
qftbx::BoxFlag BoundaryViolationDetector::classifyPoint(qftbx::NicholsPoint point, const BoundaryData * boundaries, std::size_t frequencyIndex) {

    const BoundaryColumns & columns = boundaries->columns(frequencyIndex);

    return columns.allows(columns.columnOf(point.phase), point.magnitude) ? feasible : infeasible;
}

} // namespace qftbx
