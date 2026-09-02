#ifndef QFTBX_LOOPSHAPING_LOOP_SHAPING_H
#define QFTBX_LOOPSHAPING_LOOP_SHAPING_H

#include <cstdint>
#include <memory>

#include "src/core/templates/cloud_set.h"
#include "src/core/loopshaping/algorithm_nt.h"
#include "src/core/loopshaping/algorithm_nk.h"
#include "src/core/loopshaping/algorithm_mr.h"
#include "src/core/loopshaping/algorithm_mc1.h"
#include "src/core/loopshaping/algorithm_mc_thesis.h"
#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"


/**
 * @brief Facade over the five loop-shaping algorithms: picks one, runs it
 * over the current problem, and hands back the controller it designed.
 *
 * The single point where the ownership of a designed system leaves the
 * engine, and the only thing above it that knows there are five
 * algorithms at all. What their shared epsilon argument measures is NOT
 * shared - see run().
 */
class LoopShaping
{
public:
    LoopShaping();
    ~LoopShaping();

    /**
     * @brief Runs one algorithm over the problem.
     *
     * @param plant the nominal plant.
     * @param controller the initial search box of the controller
     * parameters.
     * @param omega the design frequencies.
     * @param boundaries the QFT boundaries; MR is the one algorithm that
     * does not use them.
     * @param epsilon termination size, in the units of the algorithm
     * picked: the Nichols box diameter for NT/NK/MC1/MC, the controller
     * parameter width for MR. See
     * ProjectController::computeLoopShaping.
     * @param algorithm which of the five to run.
     * @param contour the plant template contours.
     * @param specifications the design specifications.
     * @param initialisation starting point of NK's local search.
     * @return false when the algorithm found no solution; it throws
     * qftbx::InvalidInput when the problem itself is invalid.
     */
    bool run(LtiSystem * plant, LtiSystem * controller, QVector<double> * omega, const BoundaryData * boundaries,
                 double epsilon, LoopShapingAlgorithm algorithm,
                 const qftbx::CloudSet & contour, const qftbx::SpecificationRecords * specifications,
                 std::int32_t initialisation);

    /**
     * @brief The designed controller, handed over to the caller.
     *
     * This is the single point where the ownership of a system leaves the
     * loop shaping: the facade beyond still holds it as a raw pointer.
     */
    std::unique_ptr<LtiSystem> controllerStructure();

private:

    std::unique_ptr<LtiSystem> controller;
};

#endif // QFTBX_LOOPSHAPING_LOOP_SHAPING_H
