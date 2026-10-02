/**
 * @file
 * @brief The convex hull trick on the half-line.
 *
 * The lines are sorted once: the upper envelope is the lower one of the
 * negated lines, whose order is the reverse of the lower's, and the
 * buffers of the sort and the hull are the caller's to keep. Among lines
 * that are the same in slope and intercept the one kept may differ from a
 * sort of the negated lines of their own, with the same coefficients.
 */

#include "src/core/math/line_envelope.h"

#include <algorithm>
#include <limits>

namespace qftbx {
namespace math {

namespace {

double crossing(const Line & a, const Line & b)
{
    return (b.intercept - a.intercept) / (a.slope - b.slope);
}

void envelopeOf(const std::vector<Line> & sorted, std::vector<Line> & hull, std::vector<EnvelopePiece> & pieces)
{
    pieces.clear();
    hull.clear();
    hull.reserve(sorted.size());
    for (const Line & line : sorted) {
        if (!hull.empty() && hull.back().slope == line.slope) {
            continue;
        }
        while (hull.size() >= 2) {
            const Line & first = hull[hull.size() - 2];
            const Line & second = hull.back();
            if (crossing(first, line) <= crossing(first, second)) {
                hull.pop_back();
            } else {
                break;
            }
        }
        hull.push_back(line);
    }

    double start = 0.0;
    for (std::size_t k = 0; k < hull.size(); ++k) {
        const double end = k + 1 < hull.size() ? crossing(hull[k], hull[k + 1]) : std::numeric_limits<double>::infinity();
        if (end > 0.0) {
            pieces.push_back({std::max(0.0, start), hull[k].index});
        }
        start = end;
    }
}

}

void envelopes(const std::vector<Line> & lines, EnvelopeScratch & scratch,
               std::vector<EnvelopePiece> & lower, std::vector<EnvelopePiece> & upper)
{
    std::vector<Line> & sorted = scratch.sorted;
    sorted.assign(lines.begin(), lines.end());
    std::sort(sorted.begin(), sorted.end(), [](const Line & a, const Line & b) {
        return a.slope > b.slope || (a.slope == b.slope && a.intercept < b.intercept);
    });
    envelopeOf(sorted, scratch.hull, lower);

    std::reverse(sorted.begin(), sorted.end());
    for (Line & line : sorted) {
        line.slope = -line.slope;
        line.intercept = -line.intercept;
    }
    envelopeOf(sorted, scratch.hull, upper);
}

std::vector<EnvelopePiece> lowerEnvelope(std::vector<Line> lines)
{
    EnvelopeScratch scratch;
    std::vector<EnvelopePiece> lower, upper;
    envelopes(lines, scratch, lower, upper);
    return lower;
}

std::vector<EnvelopePiece> upperEnvelope(std::vector<Line> lines)
{
    EnvelopeScratch scratch;
    std::vector<EnvelopePiece> lower, upper;
    envelopes(lines, scratch, lower, upper);
    return upper;
}

}
}
