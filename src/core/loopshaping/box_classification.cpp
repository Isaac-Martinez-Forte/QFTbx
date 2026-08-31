#include "src/core/loopshaping/box_classification.h"

BoxClassification::BoxClassification()
{
}

BoxClassification::~BoxClassification()
{
    delete m_extremes;
}

void BoxClassification::setFlag(tools::BoxFlag f)
{
    m_flag = f;
}

tools::BoxFlag BoxClassification::flag()
{
    return m_flag;
}

void BoxClassification::setExtremes(QVector<qreal> * mm)
{
    delete m_extremes;
    m_extremes = mm;
}

QVector<qreal> * BoxClassification::extremes()
{
    return m_extremes;
}

void BoxClassification::setBottomLeftForbidden(bool r)
{
    bottomLeftForbidden = r;
}

bool BoxClassification::isBottomLeftForbidden()
{
    return bottomLeftForbidden;
}

void BoxClassification::setTopRightForbidden(bool r)
{
    topRightForbidden = r;
}

bool BoxClassification::isTopRightForbidden()
{
    return topRightForbidden;
}

