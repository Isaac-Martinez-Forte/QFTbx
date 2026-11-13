/**
 * @file
 * @brief Implementation of the boundary stage.
 *
 * The prerequisite check names what is missing, and asks for the contour or
 * the whole cloud according to what the run will read. The closed-form
 * column setting, the cancellation token and the clouds on the card are
 * applied to the engine on every run, so nothing carries over from a
 * previous one; the engine is replaced when the run asks for the other
 * processor.
 */

#include "src/core/pipeline/boundary_stage.h"

#include "src/core/common/exception.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/pipeline/gpu_provider.h"

namespace qftbx {

BoundaryComputation & BoundaryStage::engine(Processor processor)
{
    if (m_engine != nullptr && m_engineProcessor == processor) {
        return *m_engine;
    }

    std::unique_ptr<BoundaryComputation> engine;
    if (processor == Processor::Cpu) {
        engine = std::make_unique<BoundaryEngine>();
    } else {
        engine = requireGpu().boundaryComputation();
        if (engine == nullptr) {
            throw ComputationError(QFTBX_TR("Core", "This build of QFTbx cannot compute the boundaries on the GPU."));
        }
    }
    m_engine = std::move(engine);
    m_engineProcessor = processor;

    return *m_engine;
}

void BoundaryStage::requirePrerequisites(const ProjectData & data,
                                         bool fromContour) const
{
    if (data.plant() == nullptr || data.frequencies() == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "The boundaries need a plant and a set of "
                           "design frequencies."));
    }
    if (data.specifications() == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "The boundaries need the specifications."));
    }
    if ((fromContour ? data.contour() : data.templates()).empty()) {
        throw InvalidInput(QFTBX_TR("Core", "The boundaries need the templates, which "
                           "have to be recomputed after the plant or "
                           "the design frequencies change."));
    }
}

bool BoundaryStage::run(ProjectData & data, Range phaseRange,
                        std::int32_t phaseCount, Range magnitudeRange,
                        std::int32_t magnitudeCount, double exportInfinity,
                        bool fromContour, Processor processor,
                        const CancellationToken * cancellation,
                        std::shared_ptr<const ResidentClouds> resident)
{
    requirePrerequisites(data, fromContour);

    BoundaryComputation & bounds = engine(processor);
    bounds.setClosedFormColumns(m_closedForm);
    bounds.setCancellation(cancellation);
    bounds.setResidentClouds(std::move(resident));

    bounds.compute(data.frequencies(), data.plant(),
                   fromContour ? data.contour() : data.templates(), fromContour,
                   data.specifications(), phaseRange, phaseCount,
                   magnitudeRange, magnitudeCount, exportInfinity);

    data.setBoundaries(bounds.boundaryData());

    return true;
}

}
