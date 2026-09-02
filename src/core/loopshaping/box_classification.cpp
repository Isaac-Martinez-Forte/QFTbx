#include "box_classification.h"

void BoxClassification::setFlag(tools::BoxFlag f)
{
    m_flag = f;
}

tools::BoxFlag BoxClassification::flag() const
{
    return m_flag;
}

void BoxClassification::setExtremes(const std::array<double, 4> & extremes)
{
    m_extremes = extremes;
}

const std::array<double, 4> & BoxClassification::extremes() const
{
    return m_extremes;
}

void BoxClassification::setBottomLeftForbidden(bool forbidden)
{
    m_bottomLeftForbidden = forbidden;
}

bool BoxClassification::isBottomLeftForbidden() const
{
    return m_bottomLeftForbidden;
}

void BoxClassification::setTopRightForbidden(bool forbidden)
{
    m_topRightForbidden = forbidden;
}

bool BoxClassification::isTopRightForbidden() const
{
    return m_topRightForbidden;
}
