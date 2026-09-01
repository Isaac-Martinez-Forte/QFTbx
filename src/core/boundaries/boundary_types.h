#ifndef QFTBX_BOUNDARY_TYPES_H
#define QFTBX_BOUNDARY_TYPES_H

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

/// The allowed-side label of each curve (see BoundaryEngine::allowedZone).
using TraceLabels = std::vector<QPoint>;

/// Per design frequency, the labels of each specification's curves.
using TraceMetadata = std::vector<std::map<QString, TraceLabels>>;

} // namespace qftbx

#endif // QFTBX_BOUNDARY_TYPES_H
