#ifndef QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
#define QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H

#include <QPointF>
#include <limits>

#include "src/core/math/sequence_vectors.h"
#include "src/core/boundaries/boundary_data.h"
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
class BoundaryViolationDetector
{
public:

    BoundaryViolationDetector();
    ~BoundaryViolationDetector();

    BoxClassification * classifyBox(cxsc::cinterval box, BoundaryData * boundaries, qint32 contador);

    /// Classifies one Nichols point (phase deg, magnitude dB) against the
    /// boundary union at design frequency 'contador' (parity test).
    tools::BoxFlag classifyPoint(QPointF punto, BoundaryData * boundaries, qint32 contador);

private:

    inline tools::BoxFlag pointVerdict(QPointF punto, QVector< QVector<QPointF> * > * interseccionHash,
                                               qint32 totalFase, bool abierta, bool arriba, qint32 numeroFases);
    inline qint32 phaseBucket(qreal x, qreal totalFase, qint32 numeroFases);
};

#endif // QFTBX_LOOPSHAPING_BOUNDARY_VIOLATION_DETECTOR_H
