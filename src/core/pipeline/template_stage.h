#ifndef QFTBX_TEMPLATE_STAGE_H
#define QFTBX_TEMPLATE_STAGE_H

#include <memory>
#include <vector>

#include "src/core/project/project_data.h"
#include "src/core/templates/template_engine.h"

namespace qftbx {

/**
 * @brief The template stage: the sweep of the plant family over the design
 * frequencies, and the epsilon-hull contour of each cloud.
 *
 * One of the classes the pipeline was split into. Every
 * stage owns the same five things, which used to be written out once per
 * stage inside ProjectController with a different shape each time: the
 * preconditions, the engine, its parameters, its outputs, and the publishing
 * of those outputs into the project.
 *
 * What a stage does NOT own is the dependency graph. Publishing templates
 * makes the boundaries built from the old ones meaningless, but deciding that
 * is the facade's job: a stage that invalidated its successors would be a
 * second place where the graph is written, and there are already too many.
 */
class TemplateStage
{
public:
    /**
     * @brief Throws InvalidInput naming the input that is missing.
     *
     * Stated instead of dereferenced, and it matters more since publishing an
     * input drops what was computed from the old one: without this a stage
     * whose inputs have just been invalidated would walk a null pointer
     * instead of saying what it needs.
     */
    void requirePrerequisites(const ProjectData & data) const;

    /**
     * @brief Sweeps the family and publishes the clouds, the contours and the
     * epsilon into the project.
     * @return true when it produced both clouds and contours.
     */
    bool run(ProjectData & data, std::vector<double> epsilon,
             ParameterGrids grids, bool cuda);

    /**
     * @brief Walks the contour again over the clouds already computed, with a
     * new epsilon, and publishes it.
     *
     * Throws InvalidInput when there is nothing to walk. That used to be a
     * null dereference: the engine is created lazily, so asking for a contour
     * before any templates existed went straight through a null pointer.
     */
    const CloudSet & recomputeContour(ProjectData & data,
                                      std::vector<double> epsilon);

    /**
     * @brief Takes templates computed elsewhere - by the persistence, on load
     * - and publishes them, feeding the engine as well.
     *
     * The engine has to be fed because it is what a later recomputeContour
     * walks: without this, recomputing the contour after LOADING a project
     * had nothing to work from. Both hold their own copy, which is the price
     * of the aliasing having gone away.
     */
    void adopt(ProjectData & data, CloudSet clouds, CloudSet contour,
               bool hasContour);

private:
    /// Created on first use and KEPT: it holds the clouds a recontour walks.
    /// That is an invariant of this stage, not a detail of its construction.
    TemplateEngine & engine();

    std::unique_ptr<TemplateEngine> m_engine;
};

} // namespace qftbx

#endif // QFTBX_TEMPLATE_STAGE_H
