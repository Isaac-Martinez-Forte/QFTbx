#include "src/gui/common/trace_segments.h"

#include <cmath>
#include <limits>

namespace qftbx {

namespace {

//How many columns of the grid a step may take before it stops being a step.
//Four, so that a curve that skips a column - which the union does where one
//specification stops binding - is still one curve.
const double kColumns = 4.0;

//The width of a column of the grid, read off the trace itself: the smallest
//phase step it takes that is not zero. Zero when every point shares a
//phase, and then nothing is a jump.
double columnWidth(const Trace & trace)
{
    double smallest = std::numeric_limits<double>::infinity();

    for (std::size_t i = 1; i < trace.size(); ++i) {
        const double step = std::abs(trace[i].phase - trace[i - 1].phase);
        if (step > 0.0 && step < smallest) {
            smallest = step;
        }
    }

    return std::isfinite(smallest) ? smallest : 0.0;
}

} // namespace

std::vector<std::size_t> segmentEnds(const Trace & trace)
{
    std::vector<std::size_t> ends;

    if (trace.empty()) {
        return ends;
    }

    const double column = columnWidth(trace);
    const double jump = kColumns * column;

    if (column > 0.0) {
        for (std::size_t i = 1; i < trace.size(); ++i) {
            if (std::abs(trace[i].phase - trace[i - 1].phase) > jump) {
                ends.push_back(i);
            }
        }
    }

    ends.push_back(trace.size());

    return ends;
}

std::vector<Trace> continuousSegments(const Trace & trace)
{
    if (trace.size() < 2) {
        return trace.empty() ? std::vector<Trace>() : std::vector<Trace>{trace};
    }

    const double column = columnWidth(trace);

    if (!(column > 0.0)) {
        return {trace};
    }

    const double jump = kColumns * column;

    std::vector<Trace> segments;
    Trace current{trace.front()};

    for (std::size_t i = 1; i < trace.size(); ++i) {
        if (std::abs(trace[i].phase - trace[i - 1].phase) > jump) {
            segments.push_back(std::move(current));
            current = Trace();
        }
        current.push_back(trace[i]);
    }

    segments.push_back(std::move(current));

    return segments;
}

} // namespace qftbx
