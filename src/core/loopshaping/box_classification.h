#ifndef QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H
#define QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H

#include "src/core/loopshaping/loop_shaping_types.h"
#include <array>

#include "src/core/math/sequence_vectors.h"

/**
 * @brief Result of classifying one projected Nichols box against the
 * boundary union at one design frequency (BoundaryViolationDetector).
 *
 * Carries the feasibility flag, the boundary extremes over the box's
 * phase span (indices 0-3: B_min/B_max in dB, C_min/C_max in degrees) and
 * the corner classifications that certify the cutting strips: the
 * bottom-left corner for the bottom and left strips, the top-right corner
 * for the top and right ones.
 */
namespace qftbx {

class BoxClassification
{
public:

    void setFlag(qftbx::BoxFlag f);
    qftbx::BoxFlag flag() const;

    void setExtremes(const std::array<double, 4> & extremes);
    const std::array<double, 4> & extremes() const;

    void setBottomLeftForbidden(bool forbidden);
    bool isBottomLeftForbidden() const;

    void setTopRightForbidden(bool forbidden);
    bool isTopRightForbidden() const;

private:

    qftbx::BoxFlag m_flag = qftbx::ambiguous;
    std::array<double, 4> m_extremes{};

    bool m_bottomLeftForbidden = false;
    bool m_topRightForbidden = false;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_BOX_CLASSIFICATION_H
