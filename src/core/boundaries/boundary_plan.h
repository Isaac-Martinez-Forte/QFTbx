/**
 * @file
 * @brief What a boundary computation has to compute, checked before any of
 * it.
 *
 * planBoundaries() checks the Nichols grid and the specifications, as every
 * boundary engine needs, and says which specifications apply at each design
 * frequency. A tracking specification that applies needs both of its
 * bounds. The checks throw InvalidInput with the message for the user.
 * sheetsUsed() says which sheets a frequency reads, in the order of
 * BoundarySheets.
 */

#ifndef QFTBX_BOUNDARY_PLAN_H
#define QFTBX_BOUNDARY_PLAN_H

#include <array>
#include <cstdint>
#include <vector>

#include "src/core/math/range.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_record.h"

namespace qftbx {

struct BoundaryPlan
{
    SpecificationSet specifications;
    std::vector<bool> tracking;
    std::vector<bool> stability;
    std::vector<bool> noise;
    std::vector<bool> outputDisturbance;
    std::vector<bool> inputDisturbance;
    std::vector<bool> controlEffort;

    std::array<bool, 5> sheetsUsed(std::size_t frequency) const;
};

BoundaryPlan planBoundaries(const std::vector<double> & omega, const SpecificationRecords & specifications,
                            Range phaseRange, std::int32_t phaseCount,
                            Range magnitudeRange, std::int32_t magnitudeCount);

}

#endif
