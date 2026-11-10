/**
 * @file
 * @brief Level-curve tracing of a boundary sheet at a fixed height.
 *
 * Given the sheet \f$D(\theta, m)\f$ sampled on the Nichols grid and the
 * cut height h in dB, already resolved by the caller (the tracking spread
 * T_U - T_L, or the specification's own bound), trace() walks the border of
 * every 8-connected region where \f$D \geq h\f$ (Moore boundary tracing)
 * and returns one trace per region, mapped from grid units back to Nichols
 * coordinates. Each trace is extended with one synthetic point before its
 * first and after its last sample so the later 1D union closes open
 * contours against the window frame. The tracer keeps a pointer to the
 * sheet, which must outlive it.
 */

#ifndef QFTBX_CONTOUR_TRACER_H
#define QFTBX_CONTOUR_TRACER_H

#include <cstdint>
#include <vector>

#include "src/core/boundaries/boundary_types.h"
#include "src/core/math/point.h"

namespace qftbx {

class ContourTracer
{
public:
    ContourTracer (double thresholdDb, const BoundarySheet & sheet);

    TraceSet trace (double phaseSpan, double magnitudeSpan,
                    double phaseBottom, double magnitudeBottom);

private:
    double m_thresholdDb = 0.0;
    const BoundarySheet * m_sheet = nullptr;
};

}

#endif
