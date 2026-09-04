#include "src/core/stages/loop_shaping_stage.h"

#include "src/core/exception.h"

namespace qftbx {

LoopShaping & LoopShapingStage::engine()
{
    if (m_engine == nullptr) {
        m_engine = std::make_unique<LoopShaping>();
    }

    return *m_engine;
}

void LoopShapingStage::requirePrerequisites(const ProjectData & data) const
{
    if (data.plant() == nullptr || data.frequencies() == nullptr) {
        throw InvalidInput("The loop shaping needs a plant and a set of "
                           "design frequencies.");
    }
    if (data.controller() == nullptr) {
        throw InvalidInput("The loop shaping needs a controller structure.");
    }
    if (data.boundaries() == nullptr) {
        throw InvalidInput("The loop shaping needs the boundaries, which "
                           "have to be recomputed after the plant, the "
                           "design frequencies, the specifications or "
                           "the templates change.");
    }
}

bool LoopShapingStage::run(ProjectData & data, double epsilon,
                           tools::LoopShapingAlgorithm algorithm,
                           Range plotRange, double pointCount,
                           std::int32_t initialisation,
                           const CancellationToken * cancellation)
{
    requirePrerequisites(data);

    LoopShaping & search = engine();

    //Set on every run, so neither a token nor a budget from a previous one
    //can linger: the engine is kept between runs.
    search.setCancellation(cancellation);
    search.setMaxLiveNodes(m_maxLiveNodes);

    const bool succeeded = search.run(data.plant(), data.controller(),
                                      data.frequencies(), data.boundaries(),
                                      epsilon, algorithm, data.contour(),
                                      data.specifications(), initialisation);

    if (!succeeded) {
        return false;
    }

    data.setLoopShapingResult(std::make_unique<LoopShapingResult>(
            search.controllerStructure(), plotRange, pointCount));

    return true;
}

} // namespace qftbx
