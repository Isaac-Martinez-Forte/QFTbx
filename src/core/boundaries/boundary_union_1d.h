#ifndef QFTBX_BOUNDARY_UNION_1D_H
#define QFTBX_BOUNDARY_UNION_1D_H

#include <QVector>

#include "src/core/boundaries/boundary_types.h"
#include <QPoint>
#include <QPointF>
#include <cmath>

#include "boundary_data.h"

namespace qftbx {

/**
 * @brief Merges the per-specification boundaries of each design frequency
 * into a single worst-case boundary.
 *
 * For every frequency the traces of the first specification seed the running
 * union; each further specification is merged against it by bucketing both
 * curves by phase and keeping, per phase, only the points that remain
 * binding (the most restrictive magnitude, honouring whether the allowed
 * region of each curve lies above or below it). The result is exposed as a
 * flat point set per frequency (unionVectors) and bucketed by phase, sorted
 * ascending by magnitude and deduplicated (unionBuckets) - the layout the
 * .qft files have always stored.
 *
 * @author Moisés Frutos Plaza
 */
class BoundaryUnion1D
{
public:
    BoundaryUnion1D();
    ~BoundaryUnion1D();

    void run(const BoundaryData * boundaries, const TraceMetadata & traceMetadata);

    UnionBuckets takeUnionBuckets();

    UnionTraces takeUnionVectors();

    std::vector<bool> takeOpenFlags();

    std::vector<bool> takeUpperFlags();

private:

    static constexpr qint32 kLayerCount = 2;
    static constexpr qreal kPhaseDegrees = 360.0;

    //Initialised here: the accessors below return these, and an
    //indeterminate pointer defeats the != nullptr guards of the callers
    //(nullptr at least fails honestly).
    UnionBuckets m_unionBuckets;
    UnionTraces m_unionVectors;

    std::vector<bool> m_openFlags;
    std::vector<bool> m_upperFlags;

    qint32 bucketIndex(qreal x, qreal totalPhase);

    void insertSorted(TraceSet & layerBuckets, qint32 index, QPointF point, qreal totalPhase);

    std::vector<TraceSet> buildLayerBuckets(const TraceSet & chosenCurves, qreal totalPhase, bool open, bool upper);

    Trace drawFirstLayer(const TraceSet & chosenCurves, const std::vector<TraceSet> & layerBuckets, qreal totalPhase, bool open1, bool open2);

    Trace drawSecondLayer(const TraceSet & chosenCurves, const std::vector<TraceSet> & layerBuckets, qreal totalPhase, bool open1, bool open2);

    Trace mergeLayers(const Trace & layer1, const Trace & layer2);

    TraceSet buildUnionBuckets(const Trace & unionPoints, qreal totalPhase, qint32 pointCount);

    qint32 bucketIndex(qreal x, qreal totalPhase, qint32 phaseCount);

    Trace sortByProximity(const Trace & points);


};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::BoundaryUnion1D;

#endif // QFTBX_BOUNDARY_UNION_1D_H
