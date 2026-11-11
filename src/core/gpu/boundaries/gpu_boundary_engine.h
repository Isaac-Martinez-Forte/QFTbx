/**
 * @file
 * @brief The boundary engine on the GPU.
 *
 * It checks the problem as the processor does (planBoundaries), computes
 * the sheets of every frequency on the card and reads them there
 * (boundariesOnCard): the traces, their labels and the columns come back,
 * and an engine of the processor does the union and, when they are on, the
 * closed-form columns. The loop value of each cell is the processor's to
 * the bit. A failure on the card is a ComputationError with CUDA's text.
 * boundariesOf() turns what one frequency's readings brought back into the
 * processor's traces, labels and columns, the nodes mapped to the Nichols
 * plane as ContourTracer maps them.
 */

#ifndef QFTBX_GPU_BOUNDARY_ENGINE_H
#define QFTBX_GPU_BOUNDARY_ENGINE_H

#include <cstdint>
#include <vector>

#include "src/core/boundaries/boundary_engine.h"
#include "src/core/gpu/boundaries/boundary_reading.h"
#include "src/core/gpu/boundaries/sheet_sweep.h"

namespace qftbx::gpu {

class GpuBoundaryEngine : public BoundaryComputation
{
public:
    void setCancellation(const CancellationToken * token) override;
    void setSingularLocusGuard(bool on) override;
    void setClosedFormColumns(bool on) override;

    void compute(std::vector<double> * omega, LtiSystem * plant, const CloudSet & templates,
                 bool templatesAreContours, const SpecificationRecords * specifications,
                 Range phaseRange, std::int32_t phaseCount,
                 Range magnitudeRange, std::int32_t magnitudeCount,
                 double exportInfinity) override;

    BoundaryData boundaryData() override;

private:
    BoundaryEngine m_processor;
    const CancellationToken * m_cancellation = nullptr;
    bool m_guard = true;
};

FrequencyBoundaries boundariesOf(const std::vector<SheetReading> & named, const std::vector<ReadingOnCard> & read,
                                Range phaseRange, std::int32_t phaseCount, Range magnitudeRange,
                                std::int32_t magnitudeCount);

std::vector<ReadingRequest> requestsOf(const std::vector<SheetReading> & named);

SheetProblem sheetProblemOf(const std::vector<double> & omega, LtiSystem & plant, const CloudSet & templates,
                            bool templatesAreContours, bool guard, const BoundaryPlan & plan,
                            Range phaseRange, std::int32_t phaseCount, Range magnitudeRange, std::int32_t magnitudeCount);

}

#endif
