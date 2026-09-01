#ifndef QFTBX_LOOPSHAPING_RESULT_H
#define QFTBX_LOOPSHAPING_RESULT_H

#include <QPointF>

#include "src/core/system/lti_system.h"

//The outcome of a loop-shaping run: the computed controller plus the
//frequency window and point count the viewer plots it over. It OWNS the
//controller: replacing it frees the previous one and the record frees the
//last one on destruction.
class LoopShapingResult
{
public:
    LoopShapingResult();
    LoopShapingResult (LtiSystem * controller, QPointF plotRange, qreal pointCount);
    ~LoopShapingResult();

    void setData (LtiSystem * controller, QPointF plotRange, qreal pointCount);

    void setData (LtiSystem * controller);

    LtiSystem * controller ();

    QPointF range ();

    qreal pointCount();

private:

    LtiSystem * m_controller = nullptr;
    QPointF m_plotRange;
    qreal m_pointCount;

    bool m_set;
};

#endif // QFTBX_LOOPSHAPING_RESULT_H
