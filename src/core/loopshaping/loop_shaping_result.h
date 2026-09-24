#ifndef QFTBX_LOOPSHAPING_RESULT_H
#define QFTBX_LOOPSHAPING_RESULT_H

#include <memory>
#include <optional>

#include "src/core/loopshaping/loop_shaping_statistics.h"
#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/project/settings.h"
#include "src/core/system/lti_system.h"

/**
 * @file
 * @brief The outcome of a loop-shaping run: the computed controller plus
 * the frequency window and point count the viewer plots it over.
 *
 * It OWNS the controller, and says so in the type. With it travel what the
 * run cost (zero for a result read from a file); the controller checked
 * against the specifications themselves, over the full template at every
 * design frequency, the last thing a run does before handing the result
 * over and what the interface shows next to the controller, absent for a
 * result read from a file and for a run whose project carried no templates
 * to check against; and what the run was asked for. A result does not mean
 * much without the last: the same problem answers 557.02 or 567.32
 * depending on how a phase between two boundary nodes was read, so the
 * algorithm, the termination tolerance, the column reading and the point
 * reading go into the file and come back, and the interface shows what
 * produced a design instead of the defaults.
 */
namespace qftbx {

class LoopShapingResult
{
public:
    LoopShapingResult (std::unique_ptr<LtiSystem> controller, qftbx::Range plotRange,
                       double pointCount);

    LtiSystem * controller () const;

    qftbx::Range range () const;

    double pointCount() const;

    const LoopShapingStatistics & statistics() const { return m_statistics; }
    void setStatistics(const LoopShapingStatistics & statistics) { m_statistics = statistics; }

    const std::optional<SpecificationCheck> & check() const { return m_check; }
    void setCheck(SpecificationCheck check) { m_check = std::move(check); }

    struct Run {
        LoopShapingAlgorithm algorithm = qftbx::nt;
        double epsilon = 0.0;
        bool conservativeColumns = false;
        Settings::Algorithms::PointReading pointReading = Settings::Algorithms::PointReading::Columns;
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

}

#endif
