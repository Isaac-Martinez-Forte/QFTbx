#ifndef QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
#define QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H

#include "src/core/loopshaping/loop_shaping_types.h"
#include <cstdint>
#include "src/core/math/point.h"
#include <limits>

#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/boundary_types.h"
#include "src/core/loopshaping/box_classification.h"

#include <cinterval.hpp>

/**
 * @class BoundaryViolationDetector
 * @brief Feasibility classification of projected Nichols boxes and points
 * against the boundary union of each design frequency (Tharewal 2005,
 * sec. 3.3.4), including the boundary extremes over the box's phase span
 * that drive the cutting equations of NT/NK/MC1/MC (fig. 5.1).
 *
 * The historical Nyquist-plane variants (detection in cartesian
 * coordinates) were tried and discarded by the thesis (secs. 4.5-4.6)
 * and are gone with the algorithms that carried them.
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
    BoxClassification classifyBox(cxsc::cinterval box, const BoundaryData * boundaries, std::size_t frequencyIndex);

    /// Classifies one Nichols point (phase deg, magnitude dB) against the
    /// boundary union at design frequency 'frequencyIndex' (parity test).
    qftbx::BoxFlag classifyPoint(qftbx::NicholsPoint point, const BoundaryData * boundaries, std::size_t frequencyIndex);

private:

    //bucketCount is the number of phase cells of the union (phaseCount - 1)
    //and phaseSpanDegrees the width of the window in degrees. They used to be
    //called totalFase and numeroFases and declared with their types CROSSED -
    //the count as a real and the SPAN AS AN INTEGER - so every call truncated
    //the window width. Exact on the default 360-degree window, which is why
    //nothing showed it; wrong on any other, and a division by zero for a
    //window under one degree.
    qftbx::BoxFlag pointVerdict(qftbx::NicholsPoint point, const qftbx::TraceSet & buckets,
                                std::int32_t bucketCount, bool above,
                                double phaseSpanDegrees);
    std::int32_t phaseBucket(double phaseDegrees, std::int32_t bucketCount, double phaseSpanDegrees);
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
