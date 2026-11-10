/**
 * @file
 * @brief The boundary engine on the GPU.
 *
 * It checks the problem as the processor does (planBoundaries), computes
 * the sheets of every frequency on the card (sheetsOnCard) and hands them
 * to an engine of the processor for what has no GPU code yet: the traces,
 * the columns and the union. The loop value of each cell is the
 * processor's to the bit. A failure on the card is a ComputationError with
 * CUDA's text.
 */

#ifndef QFTBX_GPU_BOUNDARY_ENGINE_H
#define QFTBX_GPU_BOUNDARY_ENGINE_H

#include <cstdint>
#include <vector>

#include "src/core/boundaries/boundary_engine.h"
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

SheetProblem sheetProblemOf(const std::vector<double> & omega, LtiSystem & plant, const CloudSet & templates,
                            bool templatesAreContours, bool guard, const BoundaryPlan & plan,
                            Range phaseRange, std::int32_t phaseCount, Range magnitudeRange, std::int32_t magnitudeCount);

}

#endif
