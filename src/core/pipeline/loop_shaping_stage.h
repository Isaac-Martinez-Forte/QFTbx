#ifndef QFTBX_LOOP_SHAPING_STAGE_H
#define QFTBX_LOOP_SHAPING_STAGE_H

#include <cstdint>
#include <memory>

#include "src/core/loopshaping/loop_shaping.h"
#include "src/core/project/project_data.h"
#include "src/core/math/range.h"

namespace qftbx {

/**
 * @brief The loop-shaping stage: the interval global search for a controller
 * that meets every bound at every design frequency.
 *
 * The last stage of the pipeline, and the only one with nothing downstream to
 * invalidate: its result is what the design IS.
 *
 * src/core/loopshaping contains no OpenMP at all: the branch-and-bound
 * search is entirely sequential, and it is the one that runs for tens of
 * minutes. That is why the facade runs it on a worker thread and hands it
 * the cancellation token below.
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
     *         the structure is one the algorithms cannot project - and
     *         qftbx::Cancelled when the token below is raised.
     * @param cancellation read once per node, so a run that is going to take
     *        tens of minutes can be given up on. Null means it cannot.
     *        Whoever holds it decides which thread the run happens on: see
     *        CancellationToken on why that is not the core's business.
     */
    bool run(ProjectData & data, double epsilon,
             qftbx::LoopShapingAlgorithm algorithm, Range plotRange,
             double pointCount, std::int32_t initialisation,
             const CancellationToken * cancellation = nullptr);

    /// The values the user may have changed. Applied on every run, so
    /// nothing can be left over from a previous one.
    void setSettings(const Settings & settings) { m_settings = settings; }

private:
    LoopShaping & engine();

    Settings m_settings;

    std::unique_ptr<LoopShaping> m_engine;
};

} // namespace qftbx

#endif // QFTBX_LOOP_SHAPING_STAGE_H
