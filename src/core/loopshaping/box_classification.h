#ifndef QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H
#define QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H

#include <QVector>

#include "Modelo/Herramientas/tools.h"

//Result of classifying one projected Nichols box against the boundary
//union at one design frequency (BoundaryViolationDetector): the
//feasibility flag, the boundary extremes over the box's phase span
//(B_min/B_max in dB, C_min/C_max in degrees, indices 0-3 of
//m_extremes) and the corner classifications that certify the cutting
//strips (bottomLeftForbidden/uniIzquierda: bottom-left corner infeasible;
//topRightForbidden: top-right corner infeasible; uniArriba keeps the historical
//meaning "bottom-left corner feasible" for algorithm NK's gate).
class BoxClassification
{
public:
    BoxClassification();

    ~BoxClassification();

    void setFlag(tools::BoxFlag f);
    tools::BoxFlag flag();

    void setExtremes(QVector<qreal> * mm);
    QVector<qreal> * extremes();

    void setBottomLeftForbidden(bool r);
    bool isBottomLeftForbidden();

    void setTopRightForbidden(bool r);
    bool isTopRightForbidden();

private:

    tools::BoxFlag m_flag;
    QVector<qreal> * m_extremes = nullptr;

    bool bottomLeftForbidden = false;
    bool topRightForbidden = false;
};

#endif // QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H
