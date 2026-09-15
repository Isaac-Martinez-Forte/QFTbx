#ifndef QFTBX_ALPHA_SHAPE_H
#define QFTBX_ALPHA_SHAPE_H

#include <cstdint>
#include <vector>

#include "src/core/templates/cloud_set.h"

namespace qftbx {

/**
 * @brief The boundary of the epsilon-hull of a point set by its definition
 * instead of by a walk: the edges a disc of diameter epsilon can touch from
 * outside. It is the alpha-shape of Edelsbrunner, Kirkpatrick and Seidel
 * (1983) with alpha = epsilon/2, the object Gutman, Nordin and Cohen (2007)
 * describe as a ball of radius epsilon/2 rolling round the set.
 *
 * Things to keep in mind:
 * - An edge between two points at most epsilon apart is on the boundary when
 *   one of the two discs of radius epsilon/2 through both endpoints holds no
 *   other point. That is a test per edge, so nothing can fail to close, lose
 *   a component or run out of steps: every component, every hole and every
 *   spike comes out, as loops.
 * - A point exactly on the disc does not block it. A tie therefore adds an
 *   edge, never removes one, and an extra edge only puts into the contour a
 *   template point that is there anyway.
 * - What is returned is the OUTER loop of each connected component: the
 *   face of largest area of its planar graph of boundary edges. Holes are
 *   not returned, as the walk does not return them either; a hole treated
 *   as filled only adds to the value set what the plants around it enclose.
 *   A spike is walked out and back.
 * - The points must be distinct. The cost is n times the square of the
 *   number of neighbours within epsilon: cheap at the epsilon a template
 *   asks for, quadratic in that number when epsilon is many times the
 *   spacing.
 */
struct AlphaShape
{
    /// Each loop as indices into the input, in traversal order, first index
    /// not repeated. An isolated point is a loop of one index. Loops are
    /// ordered by their rightmost point, rightmost first, and each starts
    /// at its own rightmost point.
    std::vector<std::vector<std::int32_t>> loops;
};

AlphaShape alphaShape(const ComplexCloud & points, double epsilon);

} // namespace qftbx

#endif // QFTBX_ALPHA_SHAPE_H
