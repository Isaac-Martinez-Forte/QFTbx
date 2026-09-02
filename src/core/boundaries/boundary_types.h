#ifndef QFTBX_BOUNDARY_TYPES_H
#define QFTBX_BOUNDARY_TYPES_H

#include <array>
#include <map>
#include <vector>

#include <QPoint>
#include <QPointF>
#include <QString>

namespace qftbx {

/// One boundary curve in the Nichols plane: phase in degrees, magnitude in dB.
using Trace = std::vector<QPointF>;

/// The curves of one specification at one design frequency. A boundary is
/// multivalued in general, hence several curves rather than one.
using TraceSet = std::vector<Trace>;

/**
 * @brief Every boundary of a project: per design frequency, the curves of each
 * specification, keyed by its name.
 *
 * Held BY VALUE. This was
 * `QVector<QMap<QString, QVector<QVector<QPointF> *> *> *> *`: a pointer to a
 * vector of pointers to maps of pointers to vectors of pointers to vectors.
 * Four levels of indirection, each with its own answer to who frees it, and
 * the answer lived in comments rather than in the types. BoundaryData even
 * carried an m_owns flag because the same object was sometimes a view over
 * the engine's data and sometimes the owner of a loaded project's.
 */
using BoundarySet = std::vector<std::map<QString, TraceSet>>;

/// The 1D union of all specifications at each design frequency: one curve per
/// frequency, the upper envelope the search tests against.
using UnionTraces = std::vector<Trace>;

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
 * Held BY VALUE, ~1.7 MB per frequency. This was
 * `QVector<QVector<qreal> *> *`, and the five of them travelled together in
 * one more level of indirection, freed by a nested loop in the caller.
 */
using BoundarySheet = std::vector<std::vector<double>>;

/**
 * @brief The five sheets of one design frequency, in the order the tracing
 * indexes them: 0 stability and sensor noise (they share the transfer
 * magnitude), 1 tracking, 2 output disturbance, 3 input disturbance,
 * 4 control effort.
 */
using BoundarySheets = std::array<BoundarySheet, 5>;

/// The allowed-side label of each curve (see BoundaryEngine::allowedZone).
using TraceLabels = std::vector<QPoint>;

/// Per design frequency, the labels of each specification's curves.
using TraceMetadata = std::vector<std::map<QString, TraceLabels>>;

} // namespace qftbx

#endif // QFTBX_BOUNDARY_TYPES_H
