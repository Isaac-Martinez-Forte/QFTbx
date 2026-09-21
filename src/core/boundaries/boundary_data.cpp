/**
 * @file
 * @brief Assembling and completing the boundary results.
 *
 * Columns missing for a frequency or a specification are rebuilt from the
 * traces, so a file written without them still reads whole; a frequency's
 * columns are the intersection over its specifications.
 */

#include <cstdint>
#include "src/core/boundaries/boundary_data.h"

namespace qftbx {

BoundaryData::BoundaryData(BoundarySet boundaries, std::vector<bool> openFlags,
                           std::vector<bool> upperFlags, std::int32_t phaseCount, qftbx::Range phaseRange,
                           UnionTraces unionBoundaries, UnionBuckets unionBuckets,
                           std::int32_t magnitudeCount, qftbx::Range magnitudeRange,
                           ColumnSet columns)
    : m_boundaries(std::move(boundaries)),
      m_openFlags(std::move(openFlags)),
      m_upperFlags(std::move(upperFlags)),
      m_phaseCount(phaseCount),
      m_phaseRange(phaseRange),
      m_magnitudeCount(magnitudeCount),
      m_magnitudeRange(magnitudeRange),
      m_unionBoundaries(std::move(unionBoundaries)),
      m_unionBuckets(std::move(unionBuckets)),
      m_specificationColumns(std::move(columns))
{

    m_specificationColumns.resize(m_boundaries.size());
    for (std::size_t f = 0; f < m_boundaries.size(); ++f) {
        for (const auto & entry : m_boundaries[f]) {
            if (entry.second.empty()) {
                continue;
            }
            std::map<std::string, BoundaryColumns> & own = m_specificationColumns[f];
            if (own.find(entry.first) == own.end()) {
                own[entry.first] = BoundaryColumns::fromTraces(entry.first, entry.second, m_phaseCount, m_phaseRange,
                                                               m_magnitudeCount, m_magnitudeRange);
            }
        }
    }

    m_columns.reserve(m_boundaries.size());
    for (std::size_t f = 0; f < m_boundaries.size(); ++f) {
        BoundaryColumns all(m_phaseCount, m_phaseRange);
        for (const auto & entry : m_specificationColumns[f]) {
            all.intersectWith(entry.second);
        }
        m_columns.push_back(std::move(all));
    }
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

}
