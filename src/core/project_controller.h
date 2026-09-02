#ifndef CONTROLADOR_H
#define CONTROLADOR_H


#include <memory>

#include <QHash>
#include <complex>

#include "src/core/system/lti_system.h"
#include "src/core/templates/template_engine.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/frequencies/omega.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/persistence/project_reader.h"
#include "src/persistence/project_writer.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/loopshaping/loop_shaping.h"
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
    void setPlant(std::unique_ptr<LtiSystem> plant);

    // --- step 2: the specifications ---------------------------------------

    qftbx::SpecificationRecords * specifications();
    void setSpecifications(std::optional<qftbx::SpecificationRecords> specifications);

    // --- step 3: the design frequencies -----------------------------------

    Omega * omega();
    void setOmega(std::unique_ptr<Omega> omega);

    /// The frequency values alone, the form every engine takes.
    QVector<qreal> * frequencies();

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
    bool computeTemplates(QVector<qreal> epsilon, qftbx::ParameterGrids grids, bool cuda);

    /// Recomputes only the contours, with a new epsilon.
    const qftbx::CloudSet & recomputeContour(QVector<qreal> epsilon);

    const qftbx::CloudSet & templates();
    const qftbx::CloudSet & contour();
    QVector<qreal> * epsilon();

    void setTemplates(qftbx::CloudSet templates, qftbx::CloudSet contour, bool hasContour);
    void setContour(qftbx::CloudSet contour);

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
    bool computeBoundaries(QPointF phaseRange, qint32 phaseCount, QPointF magnitudeRange,
                           qint32 magnitudeCount, qreal exportInfinity, bool useContour, bool cuda);

    BoundaryData * boundaries();
    void setBoundaries(std::optional<qftbx::BoundaryData> boundaries);

    const qftbx::UnionTraces & unionBoundaries();
    const qftbx::UnionBuckets & unionBuckets();

    // --- step 6: the controller structure ---------------------------------

    /// The controller BEING DESIGNED: its structure and the search box of
    /// its parameters.
    LtiSystem * controllerStructure();
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
    bool computeLoopShaping(qreal epsilon, tools::LoopShapingAlgorithm algorithm, QPointF plotRange,
                            qreal pointCount, qint32 initialisation = 0);

    LoopShapingResult * loopShapingResult();
    void setLoopShapingResult(std::unique_ptr<LoopShapingResult> result);

    // --- persistence ------------------------------------------------------

    /// Writes the whole project to a .qft file.
    bool save(QString path);

    /**
     * @brief Reads a .qft file into the project.
     *
     * @return per-section presence flags, in the historical order: plant,
     * specifications, omega, templates, boundaries, controller, loop
     * shaping, template contour. The caller owns the returned vector.
     */
    /// Which sections the file carried, in the historical order. By value:
    /// callers used to have to delete this, and most tests did not.
    std::vector<bool> load(QString path);

private:
    /// Publishing an input drops whatever was computed from the old one: see
    /// the note in the implementation.
    void dropTemplatesAndBelow();
    void dropBoundariesAndBelow();
    void dropLoopShaping();


    //The project contents, owned (replaces the historical DAO layer).
    qftbx::ProjectData data;

    //The three computation engines, created on first use.
    //Built on first use and kept: the template engine holds the clouds a
    //recontour works from.
    std::unique_ptr<BoundaryEngine> m_boundaryEngine;
    std::unique_ptr<TemplateEngine> m_templateEngine;
    std::unique_ptr<LoopShaping> m_loopShapingEngine;
};

#endif // CONTROLADOR_H
