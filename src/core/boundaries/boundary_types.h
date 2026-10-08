/**
 * @file
 * @brief The data types of the boundary computation.
 *
 * The curves of every specification at every design frequency, their
 * allowed-side labels, the union per frequency and its bucketing by phase,
 * the sheets of one frequency in tracing order, and one sheet as a grid of
 * closed-loop magnitudes.
 *
 * A Trace is a curve of the Nichols plane, phase in degrees and magnitude
 * in dB; a specification has several per frequency because a boundary is
 * multivalued in general. A BoundarySet keys them by specification name
 * and has one owner, by value. UnionTraces is the upper envelope of every
 * specification per frequency, the one the search tests against, and
 * UnionBuckets the same union per phase cell, each bucket sorted by
 * magnitude: the branch and bound reads it for every box, so it is the hot
 * one. NyquistTrace is the union on the complex plane (toNyquist), a type
 * of its own so it cannot stand for a Nichols trace. A BoundarySheet holds
 * the closed-loop magnitude in dB on the Nichols grid, one row per
 * magnitude and one column per phase, and its level curve at the bound is
 * the boundary. The sheets of a frequency are indexed 0 stability and
 * sensor noise (the same transfer), 1 tracking, 2 output disturbance,
 * 3 input disturbance, 4 control effort. A trace label is true when the
 * allowed region lies below the curve (BoundaryEngine::allowedZone
 * returns 1), so the union reads upper = !label.
 */

#ifndef QFTBX_BOUNDARY_TYPES_H
#define QFTBX_BOUNDARY_TYPES_H

#include <array>
#include <map>
#include <vector>

#include "src/core/math/point.h"
#include <string>

namespace qftbx {

using Trace = std::vector<NicholsPoint>;

using TraceSet = std::vector<Trace>;

using BoundarySet = std::vector<std::map<std::string, TraceSet>>;

using UnionTraces = std::vector<Trace>;

using NyquistTrace = std::vector<NyquistPoint>;

using NyquistTraces = std::vector<NyquistTrace>;

using UnionBuckets = std::vector<std::vector<Trace>>;

using BoundarySheet = std::vector<std::vector<double>>;

using BoundarySheets = std::array<BoundarySheet, 5>;

using TraceLabels = std::vector<bool>;

using TraceMetadata = std::vector<std::map<std::string, TraceLabels>>;

}

#endif
