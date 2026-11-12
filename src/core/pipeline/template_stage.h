/**
 * @file
 * @brief The template stage of the pipeline.
 *
 * Declares the stage that sweeps the plant family over the design
 * frequencies and extracts the epsilon-hull contour of each cloud. Like
 * every stage it owns its preconditions, its engine, its parameters, its
 * outputs and the publishing of them into the project; it does not own the
 * dependency graph, which the facade applies. The engine of the last run
 * that finished is kept, because it holds the clouds that a later recontour
 * with a new epsilon walks, and templates loaded from a file are fed to it
 * by adopt() for the same reason, the engine and the project each keeping
 * a copy. A run given no epsilon contours at the epsilon the engine
 * proposes on the clouds it swept, and publishes that one. An epsilon can
 * also be proposed before any sweep is published, on an engine of its own:
 * that costs one sweep, which the token can cancel, and publishes nothing.
 *
 * requirePrerequisites throws InvalidInput naming the missing input, so a
 * stage whose inputs were just invalidated says what it needs. run sweeps on
 * the processor asked for, the GPU through the installed provider, and
 * returns true when it produced both clouds and contours. recomputeContour and the
 * proposal from the published templates throw InvalidInput when there are
 * no templates. The contour options apply to every computation from then
 * on, and contourReports is empty until a contour has been computed;
 * poleCountChange is the last sweep's, empty for templates loaded from a
 * file.
 */

#ifndef QFTBX_TEMPLATE_STAGE_H
#define QFTBX_TEMPLATE_STAGE_H

#include <memory>
#include <optional>
#include "src/core/pipeline/cancellation.h"
#include "src/core/pipeline/processor.h"
#include <vector>

#include "src/core/project/project_data.h"
#include "src/core/templates/template_computation.h"

namespace qftbx {

class TemplateStage
{
public:
    void requirePrerequisites(const ProjectData & data) const;

    bool run(ProjectData & data, std::vector<double> epsilon,
             ParameterGrids grids, Processor processor,
             const CancellationToken * cancellation = nullptr);

    const CloudSet & recomputeContour(ProjectData & data,
                                      std::vector<double> epsilon);

    std::vector<EpsilonProposal> proposeEpsilon(const ProjectData & data);

    std::vector<EpsilonProposal> proposeEpsilon(const ProjectData & data,
                                                ParameterGrids grids,
                                                EpsilonMetric metric,
                                                const CancellationToken * cancellation = nullptr) const;

    void setWholeCloudStandsIn(bool standsIn) { m_wholeCloudStandsIn = standsIn; }

    void setAlphaShapeContour(bool alphaShape) { m_alphaShape = alphaShape; }
    bool alphaShapeContour() const { return m_alphaShape; }

    void setBorderSweep(bool border) { m_borderSweep = border; }
    bool borderSweep() const { return m_borderSweep; }

    const std::vector<ContourReport> & contourReports() const;
    const std::optional<PoleCountChange> & poleCountChange() const;

    void adopt(ProjectData & data, CloudSet clouds, CloudSet contour,
               bool hasContour, ParameterGrids grids = ParameterGrids());

private:
    TemplateComputation & engine();

    std::unique_ptr<TemplateComputation> m_engine;
    bool m_wholeCloudStandsIn = true;
    bool m_alphaShape = false;
    bool m_borderSweep = false;
};

}

#endif
