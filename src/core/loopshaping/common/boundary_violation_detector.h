#ifndef QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
#define QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H

#include "src/core/loopshaping/loop_shaping_types.h"
#include <cstdint>
#include "src/core/math/point.h"
#include <limits>

#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/boundary_types.h"
#include "src/core/loopshaping/common/box_classification.h"

#include "src/core/loopshaping/common/natural_interval_extension.h"

/**
 * @class BoundaryViolationDetector
 * @brief Feasibility classification of projected Nichols boxes and points
 * against the boundaries of each design frequency (Tharewal 2005,
 * sec. 3.3.4), including the boundary extremes over the box's phase span
 * that drive the cutting equations of NT/NK/MC1/MC (fig. 5.1).
 *
 * The verdicts are read off the allowed magnitude intervals of the phase
 * columns (BoundaryColumns), which carry every specification with its own
 * open or closed semantics. The parity test over the 1D union that used to
 * stand here misjudged the inside of a closed boundary whenever the union
 * had dropped an open one running under it, and the historical
 * Nyquist-plane variants (detection in cartesian coordinates) were tried
 * and discarded by the thesis (secs. 4.5-4.6); both are gone.
 *
 * @author Moisés Frutos Plaza
 * @author Isaac Martínez Forte
 */
namespace qftbx {

class BoundaryViolationDetector
{
public:
    /// Classification of one projected box; a plain value (four doubles,
    /// a flag and two corner verdicts), so there is nothing to own.
    BoxClassification classifyBox(NicholsBox box, const BoundaryData * boundaries, std::size_t frequencyIndex);

    /// Boxes classified so far, for the run statistics, and how the
    /// verdicts split: the three always add up to classifications().
    std::size_t classifications() const { return m_classifications; }
    std::size_t feasibleBoxes() const { return m_feasible; }
    std::size_t infeasibleBoxes() const { return m_infeasible; }
    std::size_t ambiguousBoxes() const { return m_ambiguous; }

    /// Classifies one Nichols point (phase deg, magnitude dB) against the
    /// boundaries at design frequency 'frequencyIndex'.
    qftbx::BoxFlag classifyPoint(qftbx::NicholsPoint point, const BoundaryData * boundaries, std::size_t frequencyIndex);

private:
    std::size_t m_classifications = 0;
    std::size_t m_feasible = 0;
    std::size_t m_infeasible = 0;
    std::size_t m_ambiguous = 0;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
