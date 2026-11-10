/**
 * @file
 * @brief What a boundary engine does, on whichever processor it runs.
 *
 * The boundary stage drives its engine through BoundaryComputation, so the
 * engine of the processor (BoundaryEngine) and that of the GPU are
 * interchangeable and give the same boundaries. compute() takes the
 * templates, contours or whole clouds, the specifications and the Nichols
 * grid, and boundaryData() is its result by value. The guard near the
 * singular locus is on unless switched off, and the closed-form columns off
 * unless switched on.
 */

#ifndef QFTBX_BOUNDARY_COMPUTATION_H
#define QFTBX_BOUNDARY_COMPUTATION_H

#include <cstdint>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/math/range.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"

namespace qftbx {

class BoundaryComputation
{
public:
    virtual ~BoundaryComputation() = default;

    virtual void setCancellation(const CancellationToken * token) = 0;
    virtual void setSingularLocusGuard(bool on) = 0;
    virtual void setClosedFormColumns(bool on) = 0;

    virtual void compute(std::vector<double> * omega, LtiSystem * plant, const CloudSet & templates,
                         bool templatesAreContours, const SpecificationRecords * specifications,
                         Range phaseRange, std::int32_t phaseCount,
                         Range magnitudeRange, std::int32_t magnitudeCount,
                         double exportInfinity) = 0;

    virtual BoundaryData boundaryData() = 0;
};

}

#endif
