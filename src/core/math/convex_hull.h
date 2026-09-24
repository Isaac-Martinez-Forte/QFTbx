#ifndef QFTBX_MATH_CONVEX_HULL_H
#define QFTBX_MATH_CONVEX_HULL_H

#include <complex>
#include <cstddef>
#include <vector>

/**
 * @file
 * @brief The vertices of the convex hull of a finite set of points in the
 * complex plane.
 *
 * The farthest point of a finite set from any point of the plane is a
 * vertex of its convex hull, so the plants that can be the farthest from
 * -L, the ones the tracking spread compares against the nearest, are the
 * hull vertices of the inverse template (Rodrigues, Chait and Hollot 1997).
 * Andrew's monotone chain: the points sorted lexicographically, a lower
 * and an upper chain built with a cross-product turn test, collinear
 * points left out. The result is the indices of the vertices in
 * counter-clockwise order, O(n log n); fewer than three distinct points
 * give whatever distinct points there are.
 */
namespace qftbx {
namespace math {

std::vector<std::size_t> convexHullVertices(const std::vector<std::complex<double>> & points);

}
}

#endif
