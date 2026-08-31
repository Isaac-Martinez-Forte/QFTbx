#ifndef QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H
#define QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H

#include <array>

#include "Modelo/Herramientas/tools.h"

//Result of classifying one projected Nichols box against the boundary
//union at one design frequency (BoundaryViolationDetector): the
//feasibility flag, the boundary extremes over the box's phase span
//(indices 0-3: B_min/B_max in dB, C_min/C_max in degrees) and the corner
//classifications that certify the cutting strips (bottom-left corner for
//the bottom and left strips, top-right corner for the top and right
//ones).
class BoxClassification
{
public:

    void setFlag(tools::BoxFlag f);
    tools::BoxFlag flag() const;

    void setExtremes(const std::array<double, 4> & extremes);
    const std::array<double, 4> & extremes() const;

    void setBottomLeftForbidden(bool forbidden);
    bool isBottomLeftForbidden() const;

    void setTopRightForbidden(bool forbidden);
    bool isTopRightForbidden() const;

private:

    tools::BoxFlag m_flag = tools::ambiguous;
    std::array<double, 4> m_extremes{};

    bool m_bottomLeftForbidden = false;
    bool m_topRightForbidden = false;
};

#endif // QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H
