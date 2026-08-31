#include "boundary_data.h"

namespace qftbx {

BoundaryData::BoundaryData(QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries,
                           QVector <bool> * openFlags, QVector <bool> * upperFlags,
                           qint32 phaseCount, QPointF phaseRange, QVector< QVector<QPointF> * > * unionBoundaries,
                           QVector< QVector< QVector<QPointF> * > * > * unionBuckets, qint32 magnitudeCount, QPointF magnitudeRange)
{
    m_boundaries = boundaries;
    m_phaseRange = phaseRange;
    m_magnitudeRange = magnitudeRange;
    m_phaseCount = phaseCount;
    m_magnitudeCount = magnitudeCount;
    m_unionBoundaries = unionBoundaries;
    m_unionBuckets = unionBuckets;
    m_openFlags = openFlags;
    m_upperFlags = upperFlags;
}



QVector<QMap<QString, QVector<QVector<QPointF> *> *> *> * BoundaryData::boundaries() const {
    return m_boundaries;
}

qint32 BoundaryData::phaseCount() const {
    return m_phaseCount;
}

qint32 BoundaryData::magnitudeCount() const {
    return m_magnitudeCount;
}

QPointF BoundaryData::phaseRange() const {
    return m_phaseRange;
}

QPointF BoundaryData::magnitudeRange() const {
    return m_magnitudeRange;
}

QVector<QVector<QPointF> *> * BoundaryData::unionBoundaries() const {
    return m_unionBoundaries;
}

QVector<QVector<QVector<QPointF> *> *> * BoundaryData::unionBuckets() const {
    return m_unionBuckets;
}

QVector <bool> * BoundaryData::openFlags() const {
    return m_openFlags;
}

QVector <bool> * BoundaryData::upperFlags() const {
    return m_upperFlags;
}









} // namespace qftbx
