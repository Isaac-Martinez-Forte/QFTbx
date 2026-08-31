#ifndef QFTBX_CONTOUR_TRACER_H
#define QFTBX_CONTOUR_TRACER_H


#include <QVector>
#include <QPointF>

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
 */
class ContourTracer
{
public:
    /// Cut threshold in dB (already resolved by the caller: the tracking
    /// spread T_U - T_L, or the specification's own bound) over the sheet.
    ContourTracer (qreal thresholdDb, QVector <QVector <qreal> *> * sheet);

    QVector <QVector <QPointF> *> * trace (qreal phasePoints, qreal magnitudePoints,
                                           qreal magnitudeShift);

#ifdef CUDA_AVAILABLE
    ContourTracer (qreal thresholdDb, float * sheet);

    QVector <QVector <QPointF> *> * trace (qreal phasePoints, qreal phaseCount, qreal magnitudePoints,
                                           qreal magnitudeCount, qreal magnitudeShift);
#endif


private:

    qreal m_thresholdDb;
    QVector <QVector <qreal> *> * m_sheet = nullptr;
#ifdef CUDA_AVAILABLE
    float * m_cudaSheet = nullptr;
#endif

    // Moore neighbourhood, clockwise from north:
    // Direction-number		Y
    //	NE-7	N-0	NW-1	|
    //	E-6	*	W-2	v
    //	SE-5	S-4	SW-3
    // X -->
    //						            N	NW	W	SW	S	SE	E	NE
    const char	kNeighbourX[8] =	{	0,	 1,	1,	1,	0,	-1,	-1,	-1	};
    const char	kNeighbourY[8] =	{  -1,	-1,	0,	1,	1,	 1,	 0,	-1	};

};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::ContourTracer;

#endif // QFTBX_CONTOUR_TRACER_H
