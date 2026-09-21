/**
 * @file
 * @brief The jump rule of the trace splitter.
 *
 * The column width is the smallest nonzero phase step the trace takes, and
 * a step over four columns is a jump: four, so that a curve that skips a
 * column where one specification stops binding is still one curve. When
 * every point shares a phase, nothing is a jump and the trace is one
 * segment.
 */

#include "src/gui/common/trace_segments.h"

#include <cmath>
#include <limits>

namespace qftbx {

namespace {

const double kColumns = 4.0;

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

}

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

}
