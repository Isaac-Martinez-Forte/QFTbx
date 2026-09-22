/**
 * @file
 * @brief The loop-shaping stage of the pipeline.
 *
 * Declares the last stage: the interval global search for a controller that
 * meets every bound at every design frequency. It has nothing downstream to
 * invalidate, since its result is what the design is. The search is entirely
 * sequential and can run for tens of minutes, which is why the facade runs
 * it on a worker thread and hands it a cancellation token that the search
 * reads once per node. A run returns false when the search ends without a
 * solution and throws when the problem itself is wrong, when no feasible
 * point exists or when the token is raised. The settings are applied on
 * every run, so nothing is left over from the previous one.
 */

#ifndef QFTBX_LOOP_SHAPING_STAGE_H
#define QFTBX_LOOP_SHAPING_STAGE_H

#include <cstdint>
#include <memory>

#include "src/core/loopshaping/loop_shaping.h"
#include "src/core/project/project_data.h"
#include "src/core/math/range.h"

namespace qftbx {

class LoopShapingStage
{
public:
    void requirePrerequisites(const ProjectData & data) const;

    bool run(ProjectData & data, double epsilon,
             qftbx::LoopShapingAlgorithm algorithm, Range plotRange,
             double pointCount, std::int32_t initialisation,
             const CancellationToken * cancellation = nullptr);

    void setSettings(const Settings & settings) { m_settings = settings; }

private:
    LoopShaping & engine();

    Settings m_settings;

    std::unique_ptr<LoopShaping> m_engine;
};

}

#endif
