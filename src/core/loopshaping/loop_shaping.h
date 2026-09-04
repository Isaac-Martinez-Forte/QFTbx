#ifndef QFTBX_LOOPSHAPING_LOOP_SHAPING_H
#define QFTBX_LOOPSHAPING_LOOP_SHAPING_H

#include "src/core/settings.h"
#include "src/core/loopshaping/cancellation.h"
#include <vector>
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
    bool run(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
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

    /**
     * @brief Installs the flag every algorithm reads once per node, so a run
     * can be given up on.
     *
     * Null by default, which is what it stays for every caller that does not
     * want to cancel. The token has to outlive run(), and run() throws
     * qftbx::Cancelled when it is raised.
     *
     * THREADS ARE NOT HERE ON PURPOSE. Whoever wants a search that does not
     * block them runs it wherever they like and holds the token; a Qt
     * application has better tools for that than a std::thread inside the
     * model. What the core owes is a search that can be interrupted.
     */
    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    /**
     * @brief The values the user may have changed, handed to whichever
     * algorithm run() builds.
     *
     * The whole struct rather than a setter per value, of which there would
     * be ten by now: the memory budget of the live-node list, the resolution
     * of the nominal stability check, and the figures the published
     * algorithms take. Each algorithm copies what it needs.
     */
    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

private:
    /// Not owned; handed to whichever algorithm run() builds.
    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;


    std::unique_ptr<LtiSystem> controller;
};

#endif // QFTBX_LOOPSHAPING_LOOP_SHAPING_H
