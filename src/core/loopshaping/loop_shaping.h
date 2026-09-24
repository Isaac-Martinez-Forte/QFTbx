#ifndef QFTBX_LOOPSHAPING_LOOP_SHAPING_H
#define QFTBX_LOOPSHAPING_LOOP_SHAPING_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include <vector>
#include <cstdint>
#include <memory>

#include "src/core/templates/cloud_set.h"
#include "src/core/loopshaping/nt/algorithm_nt.h"
#include "src/core/loopshaping/nk/algorithm_nk.h"
#include "src/core/loopshaping/mr/algorithm_mr.h"
#include "src/core/loopshaping/mc1/algorithm_mc1.h"
#include "src/core/loopshaping/mc_thesis/algorithm_mc_thesis.h"
#include "src/core/loopshaping/mc2/algorithm_mc2.h"
#include "src/core/loopshaping/mc3/algorithm_mc3.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include "src/core/system/lti_system.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/templates/parameter_grids.h"

/**
 * @file
 * @brief Facade over the loop-shaping algorithms: picks one, runs it over
 * the current problem, and hands back the controller it designed.
 *
 * The only thing above the algorithms that knows how many there are
 * (LoopShapingAlgorithm lists them), and the single point where the ownership
 * of a designed system leaves the engine: controllerStructure() hands it over.
 * run() takes the nominal plant, the controller's search box, the design
 * frequencies, the boundaries (MR is the one algorithm that does not read
 * them), the contours and specifications MR needs, and NK's starting point.
 * Its epsilon is in the units of the algorithm picked - the diameter of the
 * Nichols box for NT, NK, MC1 and the MC family, the width of the parameter
 * box for MR - so it does not mean the same across algorithms. run() returns
 * false when no solution was found and throws qftbx::InvalidInput when the
 * problem itself is invalid.
 *
 * Before a run the caller may install a cancellation token, read once per
 * node, which must outlive run(); the settings, of which each algorithm
 * copies what it needs; and the grids the plant family was swept over, which
 * the searches close the loop with before they return a design. statistics()
 * is what the last run cost.
 */
namespace qftbx {

class LoopShaping
{
public:
    bool run(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                 double epsilon, LoopShapingAlgorithm algorithm,
                 const qftbx::CloudSet & contour, const qftbx::SpecificationRecords * specifications,
                 std::int32_t initialisation);

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const { return m_statistics; }

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

private:
    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;
    qftbx::ParameterGrids m_sweep;

    std::unique_ptr<LtiSystem> m_controller;
    LoopShapingStatistics m_statistics;
};

}

#endif
