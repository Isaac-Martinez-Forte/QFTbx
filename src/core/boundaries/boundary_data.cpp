#include <cstdint>
#include "boundary_data.h"

namespace qftbx {

BoundaryData::BoundaryData(BoundarySet boundaries, std::vector<bool> openFlags,
                           std::vector<bool> upperFlags, std::int32_t phaseCount, qftbx::Range phaseRange,
                           UnionTraces unionBoundaries, UnionBuckets unionBuckets,
                           std::int32_t magnitudeCount, qftbx::Range magnitudeRange)
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

std::int32_t BoundaryData::phaseCount() const
{
    return m_phaseCount;
}

std::int32_t BoundaryData::magnitudeCount() const
{
    return m_magnitudeCount;
}

qftbx::Range BoundaryData::phaseRange() const
{
    return m_phaseRange;
}

qftbx::Range BoundaryData::magnitudeRange() const
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
