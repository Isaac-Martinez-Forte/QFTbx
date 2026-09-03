#ifndef QFTBX_LOOP_SHAPING_STAGE_H
#define QFTBX_LOOP_SHAPING_STAGE_H

#include <cstdint>
#include <memory>

#include "src/core/loopshaping/loop_shaping.h"
#include "src/core/project_data.h"
#include "src/core/range.h"

namespace qftbx {

/**
 * @brief The loop-shaping stage: the interval global search for a controller
 * that meets every bound at every design frequency.
 *
 * The last stage of the pipeline, and the only one with nothing downstream to
 * invalidate: its result is what the design IS.
 *
 * It is also where the worker thread and the cancellation are going to live
 * (plan 10.3, P5), and the reason to start with this one is measured rather
 * than assumed. src/core/loopshaping contains no OpenMP at all - the
 * branch-and-bound search is entirely sequential, and it is the one that runs
 * for tens of minutes - so moving it off the calling thread costs one core
 * before and one core after. The templates and the boundaries already fan out
 * with OpenMP, which is a different problem and a later one.
 */
class LoopShapingStage
{
public:
    /// Throws InvalidInput naming what is missing.
    void requirePrerequisites(const ProjectData & data) const;

    /**
     * @brief Runs the search and, if it succeeds, publishes the design.
     * @return false when the search finished without a solution. It THROWS
     *         when the problem itself is wrong - no feasible point exists, or
     *         the structure is one the algorithms cannot project.
     */
    bool run(ProjectData & data, double epsilon,
             tools::LoopShapingAlgorithm algorithm, Range plotRange,
             double pointCount, std::int32_t initialisation);

private:
    LoopShaping & engine();

    std::unique_ptr<LoopShaping> m_engine;
};

} // namespace qftbx

#endif // QFTBX_LOOP_SHAPING_STAGE_H
