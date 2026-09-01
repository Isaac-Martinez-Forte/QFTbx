#ifndef QFTBX_BOUNDARY_DATA_H
#define QFTBX_BOUNDARY_DATA_H

#include <QVector>
#include <QPointF>
#include <QMap>
#include <QString>

namespace qftbx {

/**
 * @brief View over one boundary computation's results, non-owning by
 * default.
 *
 * The BoundaryEngine keeps its containers as members and hands out views,
 * so destroying such a view never touches them and temporary views can be
 * created and deleted freely. The file parser instead allocates fresh
 * containers with no other owner: it builds its view with
 * takeOwnership(), and then destroying the view deep-deletes everything
 * it holds.
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

    /// Makes this view the OWNER of its containers: destroying it then
    /// deep-deletes the per-frequency maps and their traces, the flags,
    /// the union and its buckets. For producers that allocate fresh
    /// containers with no other owner (the file parser).
    void takeOwnership ();

    /**
     * @brief Neither copyable nor assignable.
     *
     * It owns a raw-pointer inventory and deep-deletes it when m_owns is set,
     * so a compiler-generated copy would give two objects the same pointers
     * and the second destructor would double-free them. Nothing in the
     * toolbox copies one - boundary sets travel by pointer - so this only
     * makes the hazard impossible instead of latent. The same Rule of Three
     * slip Parameter had.
     */
    BoundaryData(const BoundaryData &) = delete;
    BoundaryData & operator=(const BoundaryData &) = delete;

    ~BoundaryData();

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
    bool m_owns = false;
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::BoundaryData;

#endif // QFTBX_BOUNDARY_DATA_H
