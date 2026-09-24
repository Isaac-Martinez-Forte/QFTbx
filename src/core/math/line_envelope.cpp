/**
 * @file
 * @brief The convex hull trick on the half-line.
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

}

std::vector<EnvelopePiece> lowerEnvelope(std::vector<Line> lines)
{
    std::vector<EnvelopePiece> pieces;
    if (lines.empty()) {
        return pieces;
    }

    std::sort(lines.begin(), lines.end(), [](const Line & a, const Line & b) {
        return a.slope > b.slope || (a.slope == b.slope && a.intercept < b.intercept);
    });

    std::vector<Line> hull;
    for (const Line & line : lines) {
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

    std::vector<double> starts(hull.size(), 0.0);
    for (std::size_t k = 1; k < hull.size(); ++k) {
        starts[k] = crossing(hull[k - 1], hull[k]);
    }

    for (std::size_t k = 0; k < hull.size(); ++k) {
        const double end = k + 1 < hull.size() ? starts[k + 1] : std::numeric_limits<double>::infinity();
        if (end <= 0.0) {
            continue;
        }
        pieces.push_back({std::max(0.0, starts[k]), hull[k].index});
    }

    return pieces;
}

std::vector<EnvelopePiece> upperEnvelope(std::vector<Line> lines)
{
    for (Line & line : lines) {
        line.slope = -line.slope;
        line.intercept = -line.intercept;
    }
    return lowerEnvelope(std::move(lines));
}

}
}
