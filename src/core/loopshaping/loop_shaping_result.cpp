#include "src/core/loopshaping/loop_shaping_result.h"

namespace qftbx {

LoopShapingResult::LoopShapingResult(std::unique_ptr<LtiSystem> controller, qftbx::Range plotRange,
                                     double pointCount)
    : m_controller(std::move(controller)),
      m_plotRange(plotRange),
      m_pointCount(pointCount)
{
}

LtiSystem * LoopShapingResult::controller() const{
    return m_controller.get();
}

qftbx::Range LoopShapingResult::range() const{
    return m_plotRange;
}

double LoopShapingResult::pointCount() const{
    return m_pointCount;
}

} // namespace qftbx
