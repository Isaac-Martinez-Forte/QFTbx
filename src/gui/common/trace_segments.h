#ifndef QFTBX_GUI_TRACE_SEGMENTS_H
#define QFTBX_GUI_TRACE_SEGMENTS_H

#include <vector>

#include "src/core/boundaries/boundary_types.h"

namespace qftbx {

/**
 * @brief Splits a trace where it JUMPS, into the curves it is really made
 * of.
 *
 * A trace is a flat list of points, and the boundary of one design
 * frequency is not always one curve: the union of a closed stability
 * boundary and an open tracking one lands in the same list, and the walk
 * that orders that list is a nearest-neighbour one, which leaves behind
 * whatever was not nearest at the time and comes back for it at the end.
 * Drawn as one polyline, both show up as a long straight line across the
 * chart to a point that has nothing to do with its neighbours.
 *
 * The rule is the grid the trace was traced on: the points of one curve sit
 * in adjacent columns, so the smallest phase step the trace takes IS the
 * width of a column, and a step several columns wide is not a step - it is
 * the end of one curve and the beginning of another. Only the phase is
 * looked at: a boundary that stands vertically takes one column and a great
 * deal of magnitude, and that is a curve, not a jump.
 *
 * A point left alone comes back as a segment of one, which a caller draws
 * as the point it is rather than joining it to something it does not touch.
 */
std::vector<Trace> continuousSegments(const Trace & trace);

/**
 * @brief Where those segments END, as indices one past their last point.
 *
 * The same cuts, for a caller that holds the trace as two vectors of its
 * own - the loop view draws every boundary twice, on the Nichols plane and
 * on the complex one, and the two have to be cut in the same places
 * because they are the same points read differently.
 */
std::vector<std::size_t> segmentEnds(const Trace & trace);

} // namespace qftbx

#endif // QFTBX_GUI_TRACE_SEGMENTS_H
