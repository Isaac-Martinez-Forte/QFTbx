#ifndef QFTBX_BOUNDARY_DATA_H
#define QFTBX_BOUNDARY_DATA_H

#include <QVector>
#include <QPointF>
#include <QMap>
#include <QString>

namespace qftbx {

/**
 * @brief Non-owning view over one boundary computation's results.
 *
 * The containers belong to their producer (the BoundaryEngine or the file
 * parser); destroying a BoundaryData never touches them, so temporary views
 * can be created and deleted freely.
 *
 * Per design frequency, boundaries() maps each specification name (the
 * persisted keys of the .qft files: "Tracking", "Stability", "SensorNoise",
 * "OutputDisturbance", "InputDisturbance", "ControlEffort") to its traced
 * contours, unionBoundaries() holds the
 * 1D union of all specifications and unionBuckets() the same union bucketed
 * by phase, sorted by magnitude.
 */
class BoundaryData
{
public:
    BoundaryData(QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries,
                 QVector <bool> * openFlags, QVector <bool> * upperFlags,
                 qint32 phaseCount, QPointF phaseRange, QVector< QVector<QPointF> * > * unionBoundaries,
                 QVector< QVector< QVector<QPointF> * > * > * unionBuckets, qint32 magnitudeCount, QPointF magnitudeRange);

    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries () const;
    qint32 phaseCount () const;
    qint32 magnitudeCount () const;
    QPointF phaseRange () const;
    QPointF magnitudeRange () const;
    QVector<QVector<QPointF> * > * unionBoundaries () const;
    QVector< QVector< QVector<QPointF> * > * > * unionBuckets () const;
    QVector <bool> * openFlags () const;
    QVector <bool> * upperFlags () const;

private:
    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * m_boundaries;
    QVector <bool> * m_openFlags;
    QVector <bool> * m_upperFlags;
    qint32 m_phaseCount;
    QPointF m_phaseRange;
    qint32 m_magnitudeCount;
    QPointF m_magnitudeRange;
    QVector< QVector<QPointF> * > * m_unionBoundaries;
    QVector< QVector< QVector<QPointF> * > * > * m_unionBuckets;
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::BoundaryData;

#endif // QFTBX_BOUNDARY_DATA_H
