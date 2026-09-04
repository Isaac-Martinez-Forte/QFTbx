#include "src/core/stages/template_stage.h"

#include "src/core/exception.h"

namespace qftbx {

TemplateEngine & TemplateStage::engine()
{
    if (m_engine == nullptr) {
        m_engine = std::make_unique<TemplateEngine>();
    }

    return *m_engine;
}

void TemplateStage::requirePrerequisites(const ProjectData & data) const
{
    if (data.plant() == nullptr) {
        throw InvalidInput("The templates need a plant.");
    }
    if (data.omega() == nullptr) {
        throw InvalidInput("The templates need a set of design frequencies.");
    }
}

bool TemplateStage::run(ProjectData & data, std::vector<double> epsilon,
                        ParameterGrids grids, bool cuda)
{
    requirePrerequisites(data);

    TemplateEngine & sweep = engine();

    sweep.setEpsilon(epsilon);
    sweep.setGrids(std::move(grids));

    sweep.compute(data.plant(), data.omega()->values(), cuda);

    const bool produced = !sweep.clouds().empty() && !sweep.contours().empty();

    //Published straight from the engine's own copies: adopt() is for
    //clouds computed elsewhere, and would feed the engine what it already
    //holds, two copies of the template set later.
    data.setTemplates(sweep.clouds());
    if (!sweep.contours().empty()) {
        data.setContour(sweep.contours());
    }

    //The computation no longer reorders or replaces the frequencies: it is
    //enough to keep the epsilon used, for the persistence.
    data.setEpsilon(std::move(epsilon));

    return produced;
}

const CloudSet & TemplateStage::recomputeContour(ProjectData & data,
                                                 std::vector<double> epsilon)
{
    if (m_engine == nullptr || data.templates().empty()) {
        throw InvalidInput("There are no templates to walk a contour over.");
    }

    m_engine->computeContours(epsilon);

    data.setContour(m_engine->contours());
    data.setEpsilon(std::move(epsilon));

    return data.contour();
}

void TemplateStage::adopt(ProjectData & data, CloudSet clouds,
                          CloudSet contour, bool hasContour)
{
    engine().setClouds(clouds);

    data.setTemplates(std::move(clouds));

    if (hasContour) {
        data.setContour(std::move(contour));
    }
}

} // namespace qftbx
