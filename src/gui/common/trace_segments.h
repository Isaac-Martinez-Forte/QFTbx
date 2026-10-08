/**
 * @file
 * @brief Splits a boundary trace where it jumps, into the curves it is
 * made of.
 *
 * The boundary of one design frequency is not always one curve: a closed
 * stability boundary and an open tracking one land in the same list, and
 * the nearest-neighbour walk that orders it comes back at the end for what
 * it left behind. Drawn as one polyline, both show up as a straight line
 * across the chart. The rule is the grid the trace was traced on: the
 * smallest phase step is the width of a column, and a step several columns
 * wide is the end of one curve. Only the phase is looked at, since a
 * vertical boundary takes one column and much magnitude. A lone point comes
 * back as a segment of one, drawn as the point it is. segmentEnds gives
 * the same cuts as indices one past the last point of each segment, for a
 * caller that holds the same points in two planes, as the loop view does
 * with the Nichols and the complex one, and must cut both alike.
 */

#ifndef QFTBX_GUI_TRACE_SEGMENTS_H
#define QFTBX_GUI_TRACE_SEGMENTS_H

#include <vector>

#include "src/core/boundaries/boundary_types.h"

namespace qftbx {

std::vector<Trace> continuousSegments(const Trace & trace);

std::vector<std::size_t> segmentEnds(const Trace & trace);

}

#endif
