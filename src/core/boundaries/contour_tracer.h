#ifndef QFTBX_CONTOUR_TRACER_H
#define QFTBX_CONTOUR_TRACER_H

#include <cstdint>
#include <vector>

#include "src/core/boundaries/boundary_types.h"
#include "src/core/point.h"

namespace qftbx {

/**
 * @brief Traces the level curves of a boundary sheet at a fixed height.
 *
 * Given the sheet \f$D(\theta, m)\f$ sampled on the Nichols grid and the cut
 * height in dB, trace() walks the border of every 8-connected region where
 * \f$D \geq h\f$ (Moore boundary tracing) and returns one trace per region,
 * in grid units mapped back to Nichols coordinates. Each trace is extended
 * with one synthetic point before its first and after its last sample so the
 * later 1D union closes open contours against the window frame.
 *
 * The CPU sheet and the GPU sheet differ only in how a cell is read: one
 * walk serves both. The GPU overload used to carry its own copy of the walk,
 * with the threshold tests the other way round and a retrace of two-point
 * regions the CPU never did, so the two paths traced different boundaries.
 */
class ContourTracer
{
public:
    /// Cut threshold in dB (already resolved by the caller: the tracking
    /// spread T_U - T_L, or the specification's own bound) over the sheet.
    ContourTracer (double thresholdDb, const BoundarySheet & sheet);

    TraceSet trace (double phaseSpan, double magnitudeSpan,
                    double phaseBottom, double magnitudeBottom);

#ifdef CUDA_AVAILABLE
    /// The GPU sheet: a flat array, column-major (phase index times the
    /// magnitude count, plus the magnitude index).
    ContourTracer (double thresholdDb, const float * sheet);

    TraceSet trace (double phaseSpan, double phaseCount, double magnitudeSpan,
                    double magnitudeCount, double phaseBottom, double magnitudeBottom);
#endif

private:
    double m_thresholdDb = 0.0;
    const BoundarySheet * m_sheet = nullptr;
#ifdef CUDA_AVAILABLE
    const float * m_cudaSheet = nullptr;
#endif
};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::ContourTracer;

#endif // QFTBX_CONTOUR_TRACER_H
