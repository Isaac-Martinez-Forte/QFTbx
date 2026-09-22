/**
 * @file
 * @brief The facade of a QFT project over its data, engines and persistence.
 *
 * The interface never touches the core directly; every one of the seven
 * design steps maps onto a method here: enter the plant, the design
 * frequencies and the specifications, compute the templates and the
 * boundaries, enter the controller structure, run the loop shaping. A getter
 * returns null while its step is not complete. Everything the engines compute
 * is a function of the inputs above it, so publishing an input drops what
 * was computed from the old one; that dependency graph lives in this one
 * class and every publish and compute method applies it. A publish returns
 * whether it dropped anything, which it decides by comparing the new input
 * with the old by value, here and nowhere else. Publishing null is refused,
 * since removing a step is not something the pipeline does. A computation in
 * flight has the data to itself and anything that would change it throws
 * meanwhile; the loop shaping runs on a worker and can be cancelled. The
 * controller structure is the controller being designed, not
 * this class. The templates, the contours and the boundaries take the
 * choices of their dialogs, with the settings giving the defaults, and a
 * project is read whole before the current one is replaced.
 */

#ifndef QFTBX_PROJECT_CONTROLLER_H
#define QFTBX_PROJECT_CONTROLLER_H

#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "src/core/system/lti_system.h"
#include "src/core/pipeline/background_run.h"
#include "src/core/pipeline/pipeline_step.h"
#include "src/core/project/settings.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/pipeline/boundary_stage.h"
#include "src/core/pipeline/loop_shaping_stage.h"
#include "src/core/pipeline/template_stage.h"
#include "src/core/templates/template_engine.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/frequencies/omega.h"
#include <optional>

#include "src/core/project/project_data.h"

namespace qftbx {

class ProjectController
{
public:

    ProjectController();

    ~ProjectController();

    LtiSystem * plant();

    bool setPlant(std::unique_ptr<LtiSystem> plant);

    qftbx::SpecificationRecords * specifications();
    void setSpecifications(std::optional<qftbx::SpecificationRecords> specifications);

    Omega * omega();

    bool setOmega(std::unique_ptr<Omega> omega);

    bool computeTemplates(std::vector<double> epsilon, qftbx::ParameterGrids grids, bool cuda);

    const qftbx::CloudSet & recomputeContour(std::vector<double> epsilon);

    const qftbx::CloudSet & templates();
    const qftbx::CloudSet & contour();
    std::vector<double> * epsilon();

    qftbx::EpsilonMetric epsilonMetric() const;
    void setEpsilonMetric(qftbx::EpsilonMetric metric);

    std::vector<TemplateEngine::EpsilonProposal> proposeEpsilon();

    void setWholeCloudStandsIn(bool standsIn);

    void setAlphaShapeContour(bool alphaShape);
    bool alphaShapeContour() const;

    void setBorderSweep(bool border);
    bool borderSweep() const;

    void setClosedFormColumns(bool on);
    bool closedFormColumns() const;

    const std::vector<TemplateEngine::ContourReport> & contourReports() const;

    std::vector<TemplateEngine::EpsilonProposal> proposeEpsilon(qftbx::ParameterGrids grids,
                                                                qftbx::EpsilonMetric metric);

    bool computeBoundaries(qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange,
                           std::int32_t magnitudeCount, double exportInfinity, bool useContour, bool cuda);

    BoundaryData * boundaries();

    const qftbx::UnionTraces & unionBoundaries();
    const qftbx::UnionBuckets & unionBuckets();

    LtiSystem * controllerStructure();

    bool setControllerStructure(std::unique_ptr<LtiSystem> controller);

    bool computeLoopShaping(double epsilon, qftbx::LoopShapingAlgorithm algorithm, qftbx::Range plotRange,
                            double pointCount, std::int32_t initialisation = 0,
                            const qftbx::CancellationToken * cancellation = nullptr);

    void applySettings(const qftbx::Settings & settings);

    using ChangeHandler = std::function<void ()>;
    void setChangeHandler(ChangeHandler handler) { m_onChanged = std::move(handler); }

    qftbx::StepSet completed() const;

    void invalidateFrom(qftbx::Step step);

    bool startLoopShaping(double epsilon, qftbx::LoopShapingAlgorithm algorithm,
                          qftbx::Range plotRange, double pointCount,
                          std::int32_t initialisation = 0,
                          std::function<void ()> finished = std::function<void ()>());

    bool startTemplates(std::vector<double> epsilon, qftbx::ParameterGrids grids, bool cuda,
                        std::function<void ()> finished = std::function<void ()>());

    bool startBoundaries(qftbx::Range phaseRange, std::int32_t phaseCount,
                         qftbx::Range magnitudeRange, std::int32_t magnitudeCount,
                         double exportInfinity, bool contour, bool cuda,
                         std::function<void ()> finished = std::function<void ()>());

    enum class Computation { None, Templates, Boundaries, LoopShaping };

    void collectComputation();

    void cancelComputation();

    bool isComputing() const;

    void waitForComputation();

    bool lastComputationProduced() const;

    bool lastComputationCancelled() const;

    const std::string & lastComputationError() const;

    LoopShapingResult * loopShapingResult();

    void save(std::string path);

    qftbx::StepSet load(std::string path);

private:
    void dropTemplatesAndBelow();
    void dropBoundariesAndBelow();
    void dropLoopShaping();

    class Announce
    {
    public:
        explicit Announce(ProjectController & owner) : m_owner(owner)
        {
            ++m_owner.m_announcing;
        }

        ~Announce()
        {
            if (--m_owner.m_announcing == 0 && m_owner.m_onChanged) {
                m_owner.m_onChanged();
            }
        }

        Announce(const Announce &) = delete;
        Announce & operator=(const Announce &) = delete;

    private:
        ProjectController & m_owner;
    };

    ChangeHandler m_onChanged;
    int m_announcing = 0;

    qftbx::ProjectData m_data;

    void setTemplates(qftbx::CloudSet templates, qftbx::CloudSet contour, bool hasContour);
    void setBoundaries(std::optional<qftbx::BoundaryData> boundaries);
    void setLoopShapingResult(std::unique_ptr<LoopShapingResult> result);

    qftbx::BoundaryStage m_boundaries;
    qftbx::TemplateStage m_templates;
    void requireNotComputing() const;

    qftbx::LoopShapingStage m_loopShaping;

    qftbx::BackgroundRun m_background;
    qftbx::CancellationToken m_cancellation;

    Computation m_lastComputation = Computation::None;
};

}

#endif
