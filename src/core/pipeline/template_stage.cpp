/**
 * @file
 * @brief Implementation of the template stage.
 *
 * A run sweeps on an engine of its own, given the cancellation token and
 * every setting, and only once it has finished are its grids, clouds,
 * contours and epsilon published and its engine kept, so a run that is
 * cancelled or fails leaves the project and the clouds a recontour walks
 * as they were. A run publishes straight from the engine's own copies
 * rather than through adoption, which is for clouds computed elsewhere.
 * Only the epsilon used is kept for the persistence, the one given or the
 * one the engine proposed, since the sweep neither reorders nor replaces
 * the frequencies. A run counts as produced when it yields both clouds and
 * contours. A proposal before the sweep is given every sweep option before
 * it sweeps, since the border sweep changes the cloud it proposes for.
 */

#include "src/core/pipeline/template_stage.h"

#include "src/core/common/exception.h"
#include "src/core/pipeline/gpu_provider.h"
#include "src/core/templates/template_engine.h"

namespace qftbx {

namespace {

std::unique_ptr<TemplateComputation> templateComputation(Processor processor)
{
    if (processor == Processor::Cpu) {
        return std::make_unique<TemplateEngine>();
    }
    std::unique_ptr<TemplateComputation> engine = requireGpu().templateComputation();
    if (engine == nullptr) {
        throw ComputationError(QFTBX_TR("Core", "This build of QFTbx cannot compute the templates on the GPU."));
    }
    return engine;
}

}

TemplateComputation & TemplateStage::engine()
{
    if (m_engine == nullptr) {
        m_engine = std::make_unique<TemplateEngine>();
    }

    return *m_engine;
}

void TemplateStage::requirePrerequisites(const ProjectData & data) const
{
    if (data.plant() == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "The templates need a plant."));
    }
    if (data.omega() == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "The templates need a set of design frequencies."));
    }
}

bool TemplateStage::run(ProjectData & data, std::vector<double> epsilon,
                        ParameterGrids grids, Processor processor,
                        const CancellationToken * cancellation)
{
    requirePrerequisites(data);

    std::unique_ptr<TemplateComputation> sweep = templateComputation(processor);

    sweep->setCancellation(cancellation);

    sweep->setHullMetric(data.epsilonMetric().metric, data.epsilonMetric().dbPerDegree);
    sweep->setWholeCloudStandsIn(m_wholeCloudStandsIn);
    sweep->setAlphaShapeContour(m_alphaShape);
    sweep->setBorderSweep(m_borderSweep);
    sweep->setEpsilon(std::move(epsilon));
    sweep->setGrids(grids);

    sweep->compute(data.plant(), data.omega()->values());

    const bool produced = !sweep->clouds().empty() && !sweep->contours().empty();

    data.setSweepGrids(std::move(grids));
    data.setTemplates(sweep->clouds());
    if (!sweep->contours().empty()) {
        data.setContour(sweep->contours());
    }

    data.setEpsilon(sweep->epsilon());

    m_engine = std::move(sweep);

    return produced;
}

const CloudSet & TemplateStage::recomputeContour(ProjectData & data,
                                                 std::vector<double> epsilon)
{
    if (m_engine == nullptr || data.templates().empty()) {
        throw InvalidInput(QFTBX_TR("Core", "There are no templates to walk a contour over."));
    }

    m_engine->setHullMetric(data.epsilonMetric().metric, data.epsilonMetric().dbPerDegree);
    m_engine->setWholeCloudStandsIn(m_wholeCloudStandsIn);
    m_engine->setAlphaShapeContour(m_alphaShape);
    m_engine->computeContours(epsilon);

    data.setContour(m_engine->contours());
    data.setEpsilon(std::move(epsilon));

    return data.contour();
}

std::vector<EpsilonProposal> TemplateStage::proposeEpsilon(const ProjectData & data)
{
    if (m_engine == nullptr || data.templates().empty()) {
        throw InvalidInput(QFTBX_TR("Core", "There are no templates to propose an epsilon for."));
    }
    m_engine->setHullMetric(data.epsilonMetric().metric, data.epsilonMetric().dbPerDegree);
    m_engine->setAlphaShapeContour(m_alphaShape);
    return m_engine->proposeEpsilon();
}

std::vector<EpsilonProposal> TemplateStage::proposeEpsilon(const ProjectData & data,
                                                           ParameterGrids grids,
                                                           EpsilonMetric metric,
                                                           const CancellationToken * cancellation) const
{
    requirePrerequisites(data);

    TemplateEngine sweep;
    sweep.setCancellation(cancellation);
    sweep.setGrids(std::move(grids));
    sweep.setHullMetric(metric.metric, metric.dbPerDegree);
    sweep.setAlphaShapeContour(m_alphaShape);
    sweep.setBorderSweep(m_borderSweep);
    sweep.setClouds(sweep.computeClouds(data.plant(), data.omega()->values()));

    return sweep.proposeEpsilon();
}

const std::vector<ContourReport> & TemplateStage::contourReports() const
{
    static const std::vector<ContourReport> none;
    return m_engine != nullptr ? m_engine->contourReports() : none;
}

const std::optional<PoleCountChange> & TemplateStage::poleCountChange() const
{
    static const std::optional<PoleCountChange> none;
    return m_engine != nullptr ? m_engine->poleCountChange() : none;
}

void TemplateStage::adopt(ProjectData & data, CloudSet clouds,
                          CloudSet contour, bool hasContour, ParameterGrids grids)
{
    engine().setClouds(clouds);

    data.setTemplates(std::move(clouds));
    data.setSweepGrids(std::move(grids));

    if (hasContour) {
        data.setContour(std::move(contour));
    }
}

}
