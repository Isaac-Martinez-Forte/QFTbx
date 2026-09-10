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

    //Set on every run, so neither a token nor a budget from a previous one
    //can linger: the engine is kept between runs.
    search.setCancellation(cancellation);
    search.setSettings(m_settings);

    const bool succeeded = search.run(data.plant(), data.controller(),
                                      data.frequencies(), data.boundaries(),
                                      epsilon, algorithm, data.contour(),
                                      data.specifications(), initialisation);

    if (!succeeded) {
        return false;
    }

    auto result = std::make_unique<LoopShapingResult>(search.controllerStructure(), plotRange, pointCount);
    result->setStatistics(search.statistics());

    //The last step of a run: the controller the search certified against
    //the boundaries, checked against the specifications themselves over the
    //full template and at its own loop value. The boundaries are a
    //discretisation and do not all err on the safe side; this is what says
    //whether the answer actually satisfies what it was asked, and by how
    //much it misses when it does not. A project whose templates are not
    //there to check against (boundaries loaded without them) gets no check.
    if (data.templates().size() == data.frequencies()->size()) {
        result->setCheck(checkAgainstSpecifications(*result->controller(), *data.plant(),
                                                    *data.frequencies(), data.templates(),
                                                    toSpecificationSet(*data.specifications())));
    }

    data.setLoopShapingResult(std::move(result));

    return true;
}

} // namespace qftbx
