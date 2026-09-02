#ifndef QFTBX_LOOPSHAPING_RESULT_H
#define QFTBX_LOOPSHAPING_RESULT_H

#include <memory>


#include "src/core/system/lti_system.h"

/**
 * @brief The outcome of a loop-shaping run: the computed controller plus
 * the frequency window and point count the viewer plots it over.
 *
 * It OWNS the controller, and says so in the type.
 */
class LoopShapingResult
{
public:
    LoopShapingResult (std::unique_ptr<LtiSystem> controller, qftbx::Range plotRange,
                       double pointCount);

    /// Observer on the computed controller; the record keeps ownership.
    LtiSystem * controller ();

    qftbx::Range range ();

    double pointCount();

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
};

#endif // QFTBX_LOOPSHAPING_RESULT_H
