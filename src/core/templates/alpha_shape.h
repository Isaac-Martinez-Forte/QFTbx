/**
 * @file
 * @brief The boundary of the epsilon-hull of a point set, by its definition.
 *
 * It is the alpha-shape of Edelsbrunner, Kirkpatrick and Seidel (1983) with
 * alpha = epsilon/2, the ball of radius epsilon/2 rolling round the set
 * that Gutman, Nordin and Cohen (2007) describe. An edge between two points
 * at most epsilon apart belongs to the boundary when one of the two discs
 * of radius epsilon/2 through its ends holds no other point; a point
 * exactly on the disc does not block it, so a tie can only add an edge,
 * which puts in the contour a template point that is there anyway. Being a
 * test per edge, nothing can fail to close or lose a component.
 *
 * The result is the outer loop of every connected component, the face of
 * largest area of its graph of boundary edges, which is what the walk of
 * the template engine approximates and what a border sweep, a curve rather
 * than a cloud, needs computed exactly. Holes are not returned, as the walk
 * does not return them either, and a hole taken as filled only adds what
 * the plants around it enclose; a spike is walked out and back. Each loop
 * holds indices into the input in traversal order, without repeating the
 * first, an isolated point being a loop of one; loops are ordered
 * rightmost first and each starts at its own rightmost point. The points
 * must be distinct, and the cost is n times the square of the number of
 * neighbours within epsilon.
 *
 * finishedLoops() is the last stage on its own, for faces found elsewhere:
 * the largest face of each component and the isolated points in, the loops
 * out. loopsContour() turns loops into a contour, each loop's points once
 * in order and the first again to close it, and says where each starts.
 */

#ifndef QFTBX_ALPHA_SHAPE_H
#define QFTBX_ALPHA_SHAPE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "src/core/templates/cloud_set.h"

namespace qftbx {

struct AlphaShape
{
    std::vector<std::vector<std::int32_t>> loops;
};

AlphaShape alphaShape(const ComplexCloud & points, double epsilon);

std::vector<std::vector<std::int32_t>> finishedLoops(const ComplexCloud & points,
                                                     std::vector<std::vector<std::int32_t>> candidates,
                                                     const std::vector<std::int32_t> & isolated);

ComplexCloud loopsContour(const ComplexCloud & points, const std::vector<std::vector<std::int32_t>> & loops,
                          std::vector<std::size_t> * starts);

}

#endif
