/**
 * @file
 * @brief Implementation of the loop-shaping stage.
 *
 * The cancellation token and the settings are applied on every run, so
 * neither a token nor a budget lingers on the engine kept between runs. A
 * successful search ends with a direct check: the controller certified
 * against the boundaries is evaluated against the specifications themselves
 * over the full templates at its own loop value, because the boundaries are
 * a discretisation and do not all err on the safe side. That check says
 * whether the answer meets what was asked and by how much it misses when it
 * does not; a project whose templates are absent gets no check.
 */

#include "src/core/pipeline/loop_shaping_stage.h"

#include "src/core/common/exception.h"
#include "src/core/specifications/specification_record.h"

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
        throw InvalidInput(QFTBX_TR("Core", "The loop shaping needs a plant and a set of "
                           "design frequencies."));
    }
    if (data.controller() == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "The loop shaping needs a controller structure."));
    }
    if (data.boundaries() == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "The loop shaping needs the boundaries, which "
                           "have to be recomputed after the plant, the "
                           "design frequencies, the specifications or "
                           "the templates change."));
    }
}

bool LoopShapingStage::run(ProjectData & data, double epsilon,
                           qftbx::LoopShapingAlgorithm algorithm,
                           Range plotRange, double pointCount,
                           std::int32_t initialisation,
                           const CancellationToken * cancellation)
{
    requirePrerequisites(data);

    LoopShaping & search = engine();

    search.setCancellation(cancellation);
    search.setSettings(m_settings);
    search.setPlantFamily(data.sweepGrids());

    const bool succeeded = search.run(data.plant(), data.controller(),
                                      data.frequencies(), data.boundaries(),
                                      epsilon, algorithm, data.contour(),
                                      data.specifications(), initialisation);

    if (!succeeded) {
        return false;
    }

    auto result = std::make_unique<LoopShapingResult>(search.controllerStructure(), plotRange, pointCount);
    result->setStatistics(search.statistics());
    result->setRun({algorithm, epsilon, m_settings.algorithms.conservativeBoundaryColumns});

    if (data.templates().size() == data.frequencies()->size()) {
        result->setCheck(checkAgainstSpecifications(*result->controller(), *data.plant(),
                                                    *data.frequencies(), data.templates(),
                                                    toSpecificationSet(*data.specifications()),
                                                    &data.sweepGrids()));
    }

    data.setLoopShapingResult(std::move(result));

    return true;
}

}
