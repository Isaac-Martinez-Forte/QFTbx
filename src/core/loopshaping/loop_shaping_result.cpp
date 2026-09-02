#include "src/core/loopshaping/loop_shaping_result.h"

LoopShapingResult::LoopShapingResult(std::unique_ptr<LtiSystem> controller, QPointF plotRange,
                                     qreal pointCount)
    : m_controller(std::move(controller)),
      m_plotRange(plotRange),
      m_pointCount(pointCount)
{
}

LtiSystem * LoopShapingResult::controller(){
    return m_controller.get();
}

QPointF LoopShapingResult::range(){
    return m_plotRange;
}

qreal LoopShapingResult::pointCount(){
    return m_pointCount;
}
