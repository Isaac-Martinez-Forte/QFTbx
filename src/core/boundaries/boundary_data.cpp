#include "boundary_data.h"

namespace qftbx {

BoundaryData::BoundaryData(BoundarySet boundaries, std::vector<bool> openFlags,
                           std::vector<bool> upperFlags, qint32 phaseCount, QPointF phaseRange,
                           UnionTraces unionBoundaries, UnionBuckets unionBuckets,
                           qint32 magnitudeCount, QPointF magnitudeRange)
    : m_boundaries(std::move(boundaries)),
      m_openFlags(std::move(openFlags)),
      m_upperFlags(std::move(upperFlags)),
      m_phaseCount(phaseCount),
      m_phaseRange(phaseRange),
      m_magnitudeCount(magnitudeCount),
      m_magnitudeRange(magnitudeRange),
      m_unionBoundaries(std::move(unionBoundaries)),
      m_unionBuckets(std::move(unionBuckets))
{
    //No destructor: what used to be a hand-written deep delete over five
    //levels of pointers - guarded by an m_owns flag, because the same class
    //was sometimes a view and sometimes the owner - is now what the members
    //do for themselves.
}

const BoundarySet & BoundaryData::boundaries() const
{
    return m_boundaries;
}

qint32 BoundaryData::phaseCount() const
{
    return m_phaseCount;
}

qint32 BoundaryData::magnitudeCount() const
{
    return m_magnitudeCount;
}

QPointF BoundaryData::phaseRange() const
{
    return m_phaseRange;
}

QPointF BoundaryData::magnitudeRange() const
{
    return m_magnitudeRange;
}

const UnionTraces & BoundaryData::unionBoundaries() const
{
    return m_unionBoundaries;
}

const UnionBuckets & BoundaryData::unionBuckets() const
{
    return m_unionBuckets;
}

const std::vector<bool> & BoundaryData::openFlags() const
{
    return m_openFlags;
}

const std::vector<bool> & BoundaryData::upperFlags() const
{
    return m_upperFlags;
}

} // namespace qftbx
