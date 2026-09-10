#include <cstdint>
#include <limits>

#include "src/core/loopshaping/common/boundary_violation_detector.h"

namespace qftbx {

//Tharewal 2005, sec. 3.3.4 for the verdict, fig. 5.1 for the extremes; the
//semantics are in the class comment.
BoxClassification BoundaryViolationDetector::classifyBox(NicholsBox box, const BoundaryData *boundaries, std::size_t frequencyIndex) {
    ++m_classifications;

    constexpr double kInfinity = std::numeric_limits<double>::infinity();

    const BoundaryColumns & columns = boundaries->columns(frequencyIndex);

    const double minPhase = box.phaseDegrees.lower(), maxPhase = box.phaseDegrees.upper();
    const double minMag = box.magnitudeDb.lower(), maxMag = box.magnitudeDb.upper();

    //The columns the span covers, in the reading chosen (class comment).
    const std::int32_t first = m_conservative ? columns.firstColumnCovering(minPhase) : columns.columnOf(minPhase);
    const std::int32_t last = m_conservative ? columns.lastColumnCovering(maxPhase) : columns.columnOf(maxPhase);

    double minPhaseBound = std::numeric_limits<double>::max(), maxPhaseBound = std::numeric_limits<double>::lowest();

    //The strips of uniform state under and over the span.
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

        //This column's verdict on [minMag, maxMag].
        bool allowed = false, forbidden = false;
        double previousHi = -kInfinity;
        bool decided = false;

        for (std::int32_t i = 0; i < spans.count; ++i) {
            const double lo = spans.lo[i];
            const double hi = spans.hi[i];

            //A finite end is a boundary crossing; inside the box it makes it ambiguous.
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

    //B_min and B_max: infinite when the columns do not share the strip.
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

qftbx::BoxFlag BoundaryViolationDetector::classifyPoint(qftbx::NicholsPoint point, const BoundaryData * boundaries, std::size_t frequencyIndex) {

    const BoundaryColumns & columns = boundaries->columns(frequencyIndex);

    if (!m_conservative) {
        return columns.allows(columns.columnOf(point.phase), point.magnitude) ? feasible : infeasible;
    }

    //Conservative: both bracketing nodes must allow it.
    const std::int32_t first = columns.firstColumnCovering(point.phase);
    const std::int32_t last = columns.lastColumnCovering(point.phase);

    return columns.allows(first, point.magnitude) && columns.allows(last, point.magnitude)
            ? feasible : infeasible;
}

} // namespace qftbx
