/**
 * @file
 * @brief The data types of the boundary computation.
 *
 * The curves of every specification at every design frequency, their
 * allowed-side labels, the union per frequency and its bucketing by phase,
 * the sheets of one frequency in tracing order, and one sheet as a grid of
 * closed-loop magnitudes.
 */

#ifndef QFTBX_BOUNDARY_TYPES_H
#define QFTBX_BOUNDARY_TYPES_H

#include <array>
#include <map>
#include <vector>

#include "src/core/math/point.h"
#include <string>

namespace qftbx {

/// One boundary curve in the Nichols plane: phase in degrees, magnitude in dB.
using Trace = std::vector<NicholsPoint>;

/// The curves of one specification at one design frequency. A boundary is
/// multivalued in general, hence several curves rather than one.
using TraceSet = std::vector<Trace>;

/**
 * @brief Every boundary of a project: per design frequency, the curves of each
 * specification, keyed by its name.
 *
 * Held by value: one owner, always.
 */
using BoundarySet = std::vector<std::map<std::string, TraceSet>>;

/// The 1D union of all specifications at each design frequency: one curve per
/// frequency, the upper envelope the search tests against.
using UnionTraces = std::vector<Trace>;

/// One curve of the Nyquist view: the same union read on the complex plane
/// (see qftbx::toNyquist). Its own type so it cannot be passed where a
/// Nichols trace is expected.
using NyquistTrace = std::vector<NyquistPoint>;

/// Per design frequency, one Nyquist curve.
using NyquistTraces = std::vector<NyquistTrace>;

/**
 * @brief The union again, bucketed by phase: per frequency, one bucket per
 * phase cell, each holding the union points at that phase sorted by
 * magnitude.
 *
 * This is what the branch and bound reads for every box it classifies, so it
 * is the one container of this family that is genuinely hot.
 */
using UnionBuckets = std::vector<std::vector<Trace>>;

/**
 * @brief One boundary sheet: the specification's closed-loop transfer
 * magnitude in dB at every Nichols grid point, one row per magnitude and one
 * column per phase. The level curves of this surface at the specification's
 * bound are the boundary.
 *
 * Held by value, about 1.7 MB per frequency.
 */
using BoundarySheet = std::vector<std::vector<double>>;

/**
 * @brief The five sheets of one design frequency, in the order the tracing
 * indexes them: 0 stability and sensor noise (they share the transfer
 * magnitude), 1 tracking, 2 output disturbance, 3 input disturbance,
 * 4 control effort.
 */
using BoundarySheets = std::array<BoundarySheet, 5>;

/**
 * @brief The allowed-side label of each curve: true when the probe just
 * under the curve's top is allowed, so the allowed region lies BELOW the
 * curve; false when it lies above (BoundaryEngine::allowedZone returns 1
 * or 0). The 1D union reads it as upper = !label.
 */
using TraceLabels = std::vector<bool>;

/// Per design frequency, the labels of each specification's curves.
using TraceMetadata = std::vector<std::map<std::string, TraceLabels>>;

}

#endif
