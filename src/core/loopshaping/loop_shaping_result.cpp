#include "src/core/loopshaping/loop_shaping_result.h"

LoopShapingResult::LoopShapingResult(std::unique_ptr<LtiSystem> controller, qftbx::Range plotRange,
                                     double pointCount)
    : m_controller(std::move(controller)),
      m_plotRange(plotRange),
      m_pointCount(pointCount)
{
}

LtiSystem * LoopShapingResult::controller(){
    return m_controller.get();
}

qftbx::Range LoopShapingResult::range(){
    return m_plotRange;
}

double LoopShapingResult::pointCount(){
    return m_pointCount;
}
