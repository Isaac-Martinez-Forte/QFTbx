/**
 * @file
 * @brief The template stage of the pipeline.
 *
 * Declares the stage that sweeps the plant family over the design
 * frequencies and extracts the epsilon-hull contour of each cloud. Like
 * every stage it owns its preconditions, its engine, its parameters, its
 * outputs and the publishing of them into the project; it does not own the
 * dependency graph, which the facade applies. The engine is created on
 * first use and kept, because it holds the clouds that a later recontour
 * with a new epsilon walks, and templates loaded from a file are fed to it
 * for the same reason. An epsilon can also be proposed before any sweep is
 * published, on an engine of its own.
 */

#ifndef QFTBX_TEMPLATE_STAGE_H
#define QFTBX_TEMPLATE_STAGE_H

#include <memory>
#include "src/core/pipeline/cancellation.h"
#include <vector>

#include "src/core/project/project_data.h"
#include "src/core/templates/template_engine.h"

namespace qftbx {

/**
 * @brief The template stage: the sweep of the plant family over the design
 * frequencies, and the epsilon-hull contour of each cloud.
 *
 * Every stage of the pipeline owns the same five things: the preconditions,
 * the engine, its parameters, its outputs, and the publishing of those
 * outputs into the project.
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
             ParameterGrids grids, bool cuda,
             const CancellationToken * cancellation = nullptr);

    /**
     * @brief Walks the contour again over the clouds already computed, with a
     * new epsilon, and publishes it.
     *
     * Throws InvalidInput when there is nothing to walk: the engine is
     * created lazily, so asking before any templates exist finds no engine.
     */
    const CloudSet & recomputeContour(ProjectData & data,
                                      std::vector<double> epsilon);

    /// The epsilon each cloud asks for, in the project's plane, and how
    /// coarse the sweep is (TemplateEngine::proposeEpsilon). Throws
    /// InvalidInput when there are no templates.
    std::vector<TemplateEngine::EpsilonProposal> proposeEpsilon(const ProjectData & data);

    /**
     * @brief The epsilon each template WOULD ask for if the family were swept
     * over these grids, measured in this plane: the proposal the templates
     * dialog fills its field with before anything has been computed.
     *
     * Sweeps on an engine of its own, so the clouds a later recomputeContour
     * walks are untouched and nothing is published. It costs one sweep, the
     * same one run() will do next.
     */
    std::vector<TemplateEngine::EpsilonProposal> proposeEpsilon(const ProjectData & data,
                                                                ParameterGrids grids,
                                                                EpsilonMetric metric) const;

    /// What a contour that does not close becomes (TemplateEngine::
    /// setWholeCloudStandsIn): the whole cloud, or an error. Applied to every
    /// computation from now on.
    void setWholeCloudStandsIn(bool standsIn) { m_wholeCloudStandsIn = standsIn; }

    /// How the contour is extracted (TemplateEngine::setAlphaShapeContour).
    void setAlphaShapeContour(bool alphaShape) { m_alphaShape = alphaShape; }
    bool alphaShapeContour() const { return m_alphaShape; }

    /// Sweep only the border of a two-parameter box (TemplateEngine::setBorderSweep).
    void setBorderSweep(bool border) { m_borderSweep = border; }
    bool borderSweep() const { return m_borderSweep; }

    /// What the last contour computation reported, per frequency; empty when
    /// nothing has been computed.
    const std::vector<TemplateEngine::ContourReport> & contourReports() const;

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
    bool m_wholeCloudStandsIn = true;
    bool m_alphaShape = false;
    bool m_borderSweep = false;
};

}

#endif
