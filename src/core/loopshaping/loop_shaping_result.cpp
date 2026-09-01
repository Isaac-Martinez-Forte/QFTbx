#include "src/core/loopshaping/loop_shaping_result.h"

LoopShapingResult::LoopShapingResult()
{
    m_set = false;
}

LoopShapingResult::LoopShapingResult(LtiSystem * controller, QPointF plotRange, qreal pointCount){
    m_controller = controller;
    m_plotRange = plotRange;
    m_pointCount = pointCount;

    m_set = false;
}

LoopShapingResult::~LoopShapingResult(){
    delete m_controller;
}

void LoopShapingResult::setData(LtiSystem * controller, QPointF plotRange, qreal pointCount){
    //The historical version deleted the INCOMING controller instead of the
    //stored one: a recomputation freed the new result and kept the dangling
    //pointer.
    if (m_controller != controller){
        delete m_controller;
    }

    m_set = true;

    m_controller = controller;
    m_plotRange = plotRange;
    m_pointCount = pointCount;
}

void LoopShapingResult::setData(LtiSystem * controller){
    if (m_controller != controller){
        delete m_controller;
    }

    m_set = true;

    m_controller = controller;
}

LtiSystem * LoopShapingResult::controller(){
    return m_controller;
}

QPointF LoopShapingResult::range(){
    return m_plotRange;
}

qreal LoopShapingResult::pointCount(){
    return m_pointCount;
}
