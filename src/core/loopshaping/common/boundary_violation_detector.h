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
 * @brief Classifies a Nichols box, or a point, as feasible, infeasible or
 * ambiguous against the boundaries of one design frequency, and returns the
 * boundary extremes over the box's phase span that drive the cutting
 * equations of NT, NK, MC1 and MC (Tharewal 2005, sec. 3.3.4 and fig. 5.1).
 *
 * Things to keep in mind:
 * - The verdicts are read off the allowed magnitude intervals of the phase
 *   columns (BoundaryColumns), which carry every specification with its own
 *   open or closed semantics. Nothing here knows what a boundary "is".
 * - Feasible means every column the box spans allows the whole magnitude
 *   range; infeasible, every column forbids it; anything else is ambiguous,
 *   including columns that disagree.
 * - B_min and B_max are the limits of the strips of uniform state under and
 *   over the span, taken over EVERY column of the span, and infinite when
 *   the columns do not agree: then no cut applies. The bottom-left and
 *   top-right corner verdicts certify those strips.
 * - Two readings of a phase that falls between nodes: the nearest node (the
 *   published algorithms' reading, the default) and both bracketing nodes
 *   (the conservative one, a setting). The first lets a violating box
 *   through where the boundary is steep, +0.051 dB on example 2 with a
 *   1-degree grid; the second makes the strip cuts apply far less often and
 *   NT, NK and MC1 about a thousand times slower.
 *
 * @author Moisés Frutos Plaza
 * @author Isaac Martínez Forte
 */
namespace qftbx {

class BoundaryViolationDetector
{
public:
    /// Nearest-node reading by default; conservative reads both bracketing
    /// nodes (see Settings::algorithms).
    explicit BoundaryViolationDetector(bool conservative = false) : m_conservative(conservative) {}

    bool conservative() const { return m_conservative; }

    BoxClassification classifyBox(NicholsBox box, const BoundaryData * boundaries, std::size_t frequencyIndex);

    /// Run statistics; the three verdict counts add up to classifications().
    std::size_t classifications() const { return m_classifications; }
    std::size_t feasibleBoxes() const { return m_feasible; }
    std::size_t infeasibleBoxes() const { return m_infeasible; }
    std::size_t ambiguousBoxes() const { return m_ambiguous; }

    qftbx::BoxFlag classifyPoint(qftbx::NicholsPoint point, const BoundaryData * boundaries, std::size_t frequencyIndex);

private:
    bool m_conservative = false;
    std::size_t m_classifications = 0;
    std::size_t m_feasible = 0;
    std::size_t m_infeasible = 0;
    std::size_t m_ambiguous = 0;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
