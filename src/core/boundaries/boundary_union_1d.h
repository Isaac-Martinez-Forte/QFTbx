#ifndef QFTBX_BOUNDARY_UNION_1D_H
#define QFTBX_BOUNDARY_UNION_1D_H

#include <QVector>
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

    void run(BoundaryData * boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * traceMetadata);

    QVector< QVector< QVector<QPointF> * > * > * unionBuckets();

    QVector < QVector<QPointF> * > * unionVectors();

    QVector <bool> * openFlags();

    QVector <bool> * upperFlags();

private:

    static constexpr qint32 kLayerCount = 2;
    static constexpr qreal kPhaseDegrees = 360.0;

    QVector < QVector < QVector<QPointF> * > * > * m_unionBuckets;
    QVector < QVector <QPointF> * > * m_unionVectors;

    QVector<bool> * m_openFlags;
    QVector<bool> * m_upperFlags;

    qint32 bucketIndex(qreal x, qreal totalPhase);

    void insertSorted(QVector< QVector<QPointF> * > * layerBuckets, QVector<QPointF>::iterator it, qint32 index, QPointF point, qreal totalPhase);

    QVector< QVector< QVector<QPointF> * > * > * buildLayerBuckets(QVector< QVector<QPointF> * > * &chosenCurves, qreal totalPhase, bool open, bool upper);

    QVector<QPointF> * drawFirstLayer(QVector< QVector<QPointF> * > * &chosenCurves, QVector< QVector< QVector<QPointF> * > * > * &layerBuckets, qreal totalPhase, bool open1, bool open2);

    QVector<QPointF> * drawSecondLayer(QVector< QVector<QPointF> * > * &chosenCurves, QVector< QVector< QVector<QPointF> * > * > * &layerBuckets, qreal totalPhase, bool open1, bool open2);

    QVector<QPointF> * mergeLayers(QVector<QPointF> * &layer1, QVector<QPointF> * &layer2);

    QVector< QVector<QPointF> * > * buildUnionBuckets(QVector<QPointF> * &unionPoints, qreal totalPhase, qint32 pointCount);

    qint32 bucketIndex(qreal x, qreal totalPhase, qint32 phaseCount);

    QVector<QPointF> * sortByProximity(QVector<QPointF> * points);


};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::BoundaryUnion1D;

#endif // QFTBX_BOUNDARY_UNION_1D_H
