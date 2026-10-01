#ifndef QFTBX_MATH_LINE_ENVELOPE_H
#define QFTBX_MATH_LINE_ENVELOPE_H

#include <cstddef>
#include <vector>

/**
 * @file
 * @brief The lower and the upper envelope of a set of lines on the
 * non-negative half-line.
 *
 * At a fixed phase the squared distance from -L to a plant of the value set
 * is g^2 + (2 c_n g + |q_n|^2): a term common to every plant plus a line in
 * the loop magnitude g. The nearest plant is therefore the lowest of the
 * lines and the farthest the highest, and the tracking spread, which
 * compares the two, needs both envelopes as functions of g. The convex hull
 * trick gives them in O(n log n): lines sorted by slope, a line that never
 * becomes the extreme one dropped, and the breakpoints where the extreme
 * line changes are the intersections of consecutive survivors. Only g >= 0
 * is kept. A piece names, by the line's index, the line that is extreme
 * from its start to the start of the next piece, the last one to infinity;
 * the upper envelope is the lower one of the negated lines.
 */
namespace qftbx {
namespace math {

struct Line
{
    double slope = 0.0;
    double intercept = 0.0;
    std::size_t index = 0;
};

struct EnvelopePiece
{
    double from = 0.0;
    std::size_t index = 0;
};

std::vector<EnvelopePiece> lowerEnvelope(std::vector<Line> lines);

std::vector<EnvelopePiece> upperEnvelope(std::vector<Line> lines);

}
}

#endif
