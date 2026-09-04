#ifndef QFTBX_PROJECT_CONTROLLER_H
#define QFTBX_PROJECT_CONTROLLER_H


#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "src/core/system/lti_system.h"
#include "src/core/background_run.h"
#include "src/core/pipeline_step.h"
#include "src/core/settings.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/stages/boundary_stage.h"
#include "src/core/stages/loop_shaping_stage.h"
#include "src/core/stages/template_stage.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/frequencies/omega.h"
#include <optional>

#include "src/core/project_data.h"


/**
 * @class ProjectController
 * @brief The application's entry point to a QFT project: it owns the project
 * data and drives the computation engines and the persistence.
 *
 * The GUI never touches the core directly; it goes through here. The seven
 * design steps of the toolbox map onto the methods below: enter the plant,
 * the design frequencies and the specifications, compute the templates and
 * the boundaries, enter the controller structure, and run the loop shaping.
 * A getter returns nullptr while its step has not been completed.
 *
 * Naming note: controllerStructure() is the CONTROLLER BEING DESIGNED (an
 * LtiSystem), not this class - the historical name of both was
 * "Controlador".
 *
 * @author Isaac Martínez Forte
 */

class ProjectController
{
public:

    ProjectController();

    ~ProjectController();

    // --- step 1: the plant -------------------------------------------------

    LtiSystem * plant();

    /**
     * @brief Publishes a new plant and drops what was computed from the old one.
     * @return true when it DID drop something, i.e. when the new plant is not
     * the same by value as the one it replaced.
     *
     * The comparison lives here and not in the interface: it is this class
     * that decides whether the computed artefacts survive, so anywhere else
     * would be a second opinion on the same question - and the window used
     * to hold one, comparing the addresses of a fresh object and a stored
     * one, which can never match. It always invalidated, and it agreed with
     * this class only by accident.
     */
    bool setPlant(std::unique_ptr<LtiSystem> plant);

    // --- step 2: the specifications ---------------------------------------

    qftbx::SpecificationRecords * specifications();
    void setSpecifications(std::optional<qftbx::SpecificationRecords> specifications);

    // --- step 3: the design frequencies -----------------------------------

    Omega * omega();

    /**
     * @brief Publishes a new frequency set and drops what was computed from the old one.
     * @return true when it DID drop something, i.e. when the new set is not
     * the same by value as the one it replaced.
     *
     * The comparison lives here and not in the interface: it is this class
     * that decides whether the computed artefacts survive, so anywhere else
     * would be a second opinion on the same question - and the window used
     * to hold one, comparing the addresses of a fresh object and a stored
     * one, which can never match. It always invalidated, and it agreed with
     * this class only by accident.
     */
    bool setOmega(std::unique_ptr<Omega> omega);

    //frequencies() lived here too, returning omega()->values() under another
    //name. Two ways to ask the same question is one too many, and this was
    //the one nobody used: ProjectData::frequencies() is what the stages read,
    //and the interface goes through omega()->values().

    // --- step 4: the templates --------------------------------------------

    /**
     * @brief Computes the plant value set at every design frequency and its
     * contour.
     *
     * @param epsilon per-frequency epsilon of the contour walk.
     * @param grids sweep grid of every uncertain parameter, keyed by NAME.
     * @param cuda run the GPU path (requires a CUDA build).
     * @return false when either the clouds or the contours came out empty.
     */
    bool computeTemplates(std::vector<double> epsilon, qftbx::ParameterGrids grids, bool cuda);

    /// Recomputes only the contours, with a new epsilon.
    const qftbx::CloudSet & recomputeContour(std::vector<double> epsilon);

    const qftbx::CloudSet & templates();
    const qftbx::CloudSet & contour();
    std::vector<double> * epsilon();


    // --- step 5: the boundaries -------------------------------------------

    /**
     * @brief Computes the QFT boundaries over the Nichols grid.
     *
     * @param phaseRange, phaseCount phase axis of the grid (degrees).
     * @param magnitudeRange, magnitudeCount magnitude axis (dB).
     * @param exportInfinity finite stand-in for infinity when the results are
     * exported; < 0 means none (it takes no part in the computation).
     * @param useContour feed the engine the template contours instead of the
     * full value sets.
     * @param cuda run the GPU path.
     */
    bool computeBoundaries(qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange,
                           std::int32_t magnitudeCount, double exportInfinity, bool useContour, bool cuda);

    BoundaryData * boundaries();

    const qftbx::UnionTraces & unionBoundaries();
    const qftbx::UnionBuckets & unionBuckets();

    // --- step 6: the controller structure ---------------------------------

    /// The controller BEING DESIGNED: its structure and the search box of
    /// its parameters.
    LtiSystem * controllerStructure();

    /**
     * @brief Publishes a new controller structure and drops what was computed from the old one.
     * @return true when it DID drop something, i.e. when the new structure is not
     * the same by value as the one it replaced.
     *
     * The comparison lives here and not in the interface: it is this class
     * that decides whether the computed artefacts survive, so anywhere else
     * would be a second opinion on the same question - and the window used
     * to hold one, comparing the addresses of a fresh object and a stored
     * one, which can never match. It always invalidated, and it agreed with
     * this class only by accident.
     */
    bool setControllerStructure(std::unique_ptr<LtiSystem> controller);

    // --- step 7: the loop shaping -----------------------------------------

    /**
     * @brief Runs the selected loop-shaping algorithm over the current
     * problem.
     *
     * @param epsilon termination size of the interval search. WHAT it
     * measures depends on the algorithm, because each one follows the
     * criterion of its own paper: NT, NK, MC1 and MC stop on the diameter
     * of the Nichols box (they work on that projection), MR on the width of
     * the controller parameter box (its paper solves an ICSP over the
     * parameters). The same number is therefore not comparable across
     * algorithms - on a plant whose |P| reaches 1e4, the Nichols reading is
     * four decades tighter than the parameter one.
     * @param algorithm which of the five algorithms to run.
     * @param plotRange, pointCount frequency window the result is plotted
     * over (stored with the result, not used by the search).
     * @param initialisation starting point of NK's local search.
     * @return false when the algorithm found no solution; it throws
     * qftbx::InvalidInput when the problem itself is invalid or infeasible.
     */
    bool computeLoopShaping(double epsilon, tools::LoopShapingAlgorithm algorithm, qftbx::Range plotRange,
                            double pointCount, std::int32_t initialisation = 0,
                            const qftbx::CancellationToken * cancellation = nullptr);

    /**
     * @brief Applies the settings the application read.
     *
     * Only what the CORE needs: today that is the search's memory budget.
     * The interface keeps its own copy for its dialogs' ceilings. Called once
     * after construction; the compiled defaults stand until it is.
     */
    void applySettings(const qftbx::Settings & settings);

    // --- the pipeline as data ----------------------------------------------

    /**
     * @brief Which steps are done, DERIVED from what the project holds.
     *
     * Nothing stores this. Every one of the seven is a question the data
     * already answer - the templates are done exactly when templates() is not
     * empty - and the window used to keep seven booleans saying the same
     * thing by hand. Duplicate state is state that can go out of sync.
     */
    qftbx::StepSet completed() const;

    /**
     * @brief Drops everything computed from the given step downwards.
     *
     * The cascade in one place. The dependency order lives in this class and
     * nowhere else - or rather, that is what this is for: the window mirrors
     * the same order by hand today, and this is what it can ask instead.
     */
    void invalidateFrom(qftbx::Step step);

    // --- the search, off the calling thread --------------------------------
    //
    //The same computation as above, started on a worker and left to run. It
    //lives HERE and not in the interface because this class owns the project
    //data: while a search is in flight nothing may touch them, and the only
    //place that can actually enforce that is the one holding them. Every
    //mutating entry point refuses while a run is in flight, which is the
    //whole reason the threading is in the core rather than in the window.
    //
    //WHAT THE WORKER TOUCHES, exactly, because "do not touch the project" is
    //too vague to build an interface on:
    //  - it READS the plant, the controller structure, the frequencies, the
    //    boundaries, the contour and the specifications. Reading those from
    //    another thread at the same time is a read against a read, so a
    //    viewer may refresh from them while the search runs.
    //  - it WRITES exactly one thing, once, at the very end: the
    //    loop-shaping result. So loopShapingResult() is the one getter that
    //    must not be read until the run is over - isComputing() says when,
    //    and waitForComputation() waits.
    //  - and it MUTATES nothing else, which is what the guard enforces
    //    rather than asks for.
    //
    //start() is not safe against itself: one caller starts runs. It is safe
    //against everything else, which is what matters here.

    /**
     * @brief Starts the search on a worker thread.
     * @param finished called when it ends, however it ends - ON THE WORKER
     *        THREAD. In a Qt application that means a queued invocation, and
     *        that is the interface's business; a caller who would rather not
     *        deal with it can leave it empty and poll isComputing().
     * @return false when a computation is already in flight, in which case
     *         nothing is started.
     *
     * It throws the preconditions BEFORE starting - a search with no
     * boundaries is refused on the caller's thread, where the caller can see
     * it, not two lines into a worker.
     */
    bool startLoopShaping(double epsilon, tools::LoopShapingAlgorithm algorithm,
                          qftbx::Range plotRange, double pointCount,
                          std::int32_t initialisation = 0,
                          std::function<void ()> finished = std::function<void ()>());

    /// Asks the search in flight to stop. Safe at any time, from any thread;
    /// does nothing when there is no run.
    void cancelComputation();

    /// Whether a computation started here is in flight.
    bool isComputing() const;

    /// Blocks until the run in flight finishes. For tests and for shutdown.
    void waitForComputation();

    //The three below describe the last FINISHED run, and they are only
    //meaningful once it has finished: ask isComputing() first, or call
    //waitForComputation(). What publishes them is the release of the running
    //flag and the join, so reading them mid-run is reading a value that is
    //being written.

    /// Whether the last finished run published a design.
    bool lastComputationProduced() const;

    /// Whether the last finished run ended by being cancelled.
    bool lastComputationCancelled() const;

    /// What the last finished run threw, or empty when it threw nothing.
    const std::string & lastComputationError() const;

    LoopShapingResult * loopShapingResult();

    // --- persistence ------------------------------------------------------

    /// Writes the whole project to a .qft file.
    bool save(std::string path);

    /**
     * @brief Reads a .qft file into the project.
     *
     * @return per-section presence flags, in the historical order: plant,
     * specifications, omega, templates, boundaries, controller, loop
     * shaping, template contour. The caller owns the returned vector.
     */
    /// Which sections the file carried, in the historical order. By value:
    /// callers used to have to delete this, and most tests did not.
    qftbx::StepSet load(std::string path);

private:
    /// Publishing an input drops whatever was computed from the old one: see
    /// the note in the implementation.
    void dropTemplatesAndBelow();
    void dropBoundariesAndBelow();
    void dropLoopShaping();


    //The project contents, owned (replaces the historical DAO layer).
    qftbx::ProjectData m_data;

    //The publishers load() uses to put a file's artefacts in place. Private:
    //publishing a computed artefact from outside would bypass the dependency
    //graph, and nothing outside ever did.
    void setTemplates(qftbx::CloudSet templates, qftbx::CloudSet contour, bool hasContour);
    void setBoundaries(std::optional<qftbx::BoundaryData> boundaries);
    void setLoopShapingResult(std::unique_ptr<LoopShapingResult> result);

    //The three computation engines, created on first use.
    //Built on first use and kept: the template engine holds the clouds a
    //recontour works from.
    qftbx::BoundaryStage m_boundaries;
    //One stage per phase of the pipeline: each owns its preconditions, its
    //engine, its parameters and the publishing of its outputs. This class
    //keeps the data and the dependency graph, and delegates the rest.
    qftbx::TemplateStage m_templates;
    /// Throws InvalidInput when a computation is in flight.
    void requireNotComputing() const;

    qftbx::LoopShapingStage m_loopShaping;

    //The worker and the flag it reads. Both live as long as this class, so a
    //token cannot outlive the search that reads it.
    qftbx::BackgroundRun m_background;
    qftbx::CancellationToken m_cancellation;
};

#endif // QFTBX_PROJECT_CONTROLLER_H
