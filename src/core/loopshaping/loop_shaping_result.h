#ifndef QFTBX_LOOPSHAPING_RESULT_H
#define QFTBX_LOOPSHAPING_RESULT_H

#include <memory>
#include <optional>


#include "src/core/loopshaping/loop_shaping_statistics.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/system/lti_system.h"

/**
 * @brief The outcome of a loop-shaping run: the computed controller plus
 * the frequency window and point count the viewer plots it over.
 *
 * It OWNS the controller, and says so in the type.
 */
namespace qftbx {

class LoopShapingResult
{
public:
    LoopShapingResult (std::unique_ptr<LtiSystem> controller, qftbx::Range plotRange,
                       double pointCount);

    /// Observer on the computed controller; the record keeps ownership.
    LtiSystem * controller () const;

    qftbx::Range range () const;

    double pointCount() const;

    /// What the run that produced this result cost; zero for a result read
    /// from a file.
    const LoopShapingStatistics & statistics() const { return m_statistics; }
    void setStatistics(const LoopShapingStatistics & statistics) { m_statistics = statistics; }

    /// The controller checked against the specifications themselves, over
    /// the full template, at every design frequency: the last thing a run
    /// does before it hands the result over, and what the interface shows
    /// next to the controller. Absent for a result read from a file, and
    /// for a run whose project carried no templates to check against.
    const std::optional<SpecificationCheck> & check() const { return m_check; }
    void setCheck(SpecificationCheck check) { m_check = std::move(check); }

    /**
     * @brief What the run was asked for: the algorithm, the termination
     * tolerance, and how it was told to read a phase between two boundary
     * nodes.
     *
     * A result does not mean much without them - the same problem answers
     * 557.02 or 567.32 depending on the reading alone - so they travel with
     * it into the file and back, and the interface shows what produced a
     * design instead of the defaults.
     */
    struct Run {
        LoopShapingAlgorithm algorithm = qftbx::nt;
        double epsilon = 0.0;
        bool conservativeColumns = false;
    };

    const Run & run() const { return m_run; }
    void setRun(const Run & run) { m_run = run; }

private:

    std::unique_ptr<LtiSystem> m_controller;
    qftbx::Range m_plotRange;
    double m_pointCount = 0;
    LoopShapingStatistics m_statistics;
    std::optional<SpecificationCheck> m_check;
    Run m_run;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_RESULT_H
