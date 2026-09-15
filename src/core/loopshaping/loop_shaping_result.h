#ifndef QFTBX_LOOPSHAPING_RESULT_H
#define QFTBX_LOOPSHAPING_RESULT_H

#include <memory>
#include <optional>


#include "src/core/loopshaping/loop_shaping_statistics.h"
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

    //There was a pair of setData() overloads and an m_set flag: nothing
    //ever called them and nothing ever read the flag. The overloads
    //carried a fixed bug (they used to delete the INCOMING controller
    //instead of the stored one, so a recomputation freed the new result
    //and kept the dangling pointer), which is reason enough not to leave
    //them lying around unused.

private:

    std::unique_ptr<LtiSystem> m_controller;
    qftbx::Range m_plotRange;
    double m_pointCount = 0;
    LoopShapingStatistics m_statistics;
    std::optional<SpecificationCheck> m_check;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_RESULT_H
