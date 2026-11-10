/**
 * @file
 * @brief The sweep of a plant family, worked out before it runs, and its
 * run on the processor.
 *
 * planSweep() does everything but evaluating the plants, and refuses what
 * cannot be swept:
 * - it puts the grids of the uncertain parameters in the order the plant
 *   names them;
 * - it applies the border sweep;
 * - it says where each coefficient comes from, a digit of the combination
 *   or its nominal value;
 * - it refuses a design frequency on a pole of the nominal plant and a
 *   family that changes its number of right half-plane poles;
 * - it marks the bands that poles on the imaginary axis sweep.
 *
 * A combination's digits are its grids' values, the first parameter
 * changing fastest, or a point of the border. sweepOnProcessor() evaluates
 * every combination at every frequency, one frequency per thread, and any
 * engine that evaluates them elsewhere refuses the plants that are not
 * finite with refuseNonFinite(), the same sentence.
 */

#ifndef QFTBX_SWEEP_PLAN_H
#define QFTBX_SWEEP_PLAN_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/parameter_grids.h"

namespace qftbx {

struct SweepPlan
{
    std::vector<std::vector<double>> grids;
    std::size_t combinationCount = 0;
    std::vector<std::array<double, 2>> border;

    std::vector<std::int32_t> numeratorSlot;
    std::vector<std::int32_t> denominatorSlot;
    std::vector<double> numeratorNominal;
    std::vector<double> denominatorNominal;
    std::int32_t gainSlot = -1;
    std::int32_t delaySlot = -1;
    double gainNominal = 0.0;
    double delayNominal = 0.0;

    std::optional<int> familyRhpPoles;
    std::vector<std::optional<std::array<double, 2>>> resonanceBands;

    bool borderSweepApplied() const { return !border.empty(); }
};

SweepPlan planSweep(LtiSystem & plant, const std::vector<double> & frequencies, const ParameterGrids & grids,
                    bool borderSweep);

CloudSet sweepOnProcessor(LtiSystem & plant, const std::vector<double> & frequencies, const SweepPlan & plan,
                          const CancellationToken * cancellation);

void refuseNonFinite(const std::vector<double> & frequencies, const std::vector<char> & nonFinite);

}

#endif
