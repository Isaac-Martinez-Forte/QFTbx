/**
 * @file
 * @brief The boundary stage of the pipeline.
 *
 * Declares the stage that computes the QFT bounds on the Nichols plane, one
 * set per design frequency plus their union, over a given phase and
 * magnitude grid. It owns its preconditions, its engine, its parameters and
 * the publishing of its output, and not the dependency graph: a new set of
 * boundaries voids the design found against the old ones, but the facade
 * applies that. Which template data must be present depends on whether the
 * computation reads each template's contour or the whole cloud, and a
 * missing one throws InvalidInput naming it. run() answers whether it
 * produced a set, computed on the processor asked for, the GPU through the
 * installed provider, which reads the clouds where the card holds them when
 * it is given them (TemplateStage::cloudsOnCard); the choice of closed-form
 * columns for the magnitude specifications applies to every computation.
 */

#ifndef QFTBX_BOUNDARY_STAGE_H
#define QFTBX_BOUNDARY_STAGE_H

#include <cstdint>
#include "src/core/pipeline/cancellation.h"
#include "src/core/pipeline/processor.h"
#include <memory>

#include "src/core/boundaries/boundary_computation.h"
#include "src/core/project/project_data.h"
#include "src/core/math/range.h"

namespace qftbx {

class BoundaryStage
{
public:
    void requirePrerequisites(const ProjectData & data, bool fromContour) const;

    bool run(ProjectData & data, Range phaseRange, std::int32_t phaseCount,
             Range magnitudeRange, std::int32_t magnitudeCount,
             double exportInfinity, bool fromContour, Processor processor,
             const CancellationToken * cancellation = nullptr,
             std::shared_ptr<const ResidentClouds> resident = nullptr);

    void setClosedFormColumns(bool on) { m_closedForm = on; }
    bool closedFormColumns() const { return m_closedForm; }

private:
    BoundaryComputation & engine(Processor processor);
    bool m_closedForm = false;

    std::unique_ptr<BoundaryComputation> m_engine;
    Processor m_engineProcessor = Processor::Cpu;
};

}

#endif
