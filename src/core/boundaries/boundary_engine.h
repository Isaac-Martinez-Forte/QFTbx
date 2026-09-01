#ifndef QFTBX_BOUNDARY_ENGINE_H
#define QFTBX_BOUNDARY_ENGINE_H

#include <QVector>

#include "src/core/templates/cloud_set.h"
#include <complex>
#include <qmath.h>
#include <limits>
#include <cmath>
#ifdef OpenMP_AVAILABLE
#include <omp.h>
#endif
#ifdef CUDA_AVAILABLE
#include "src/core/gpu/boundary_sheets_cuda.h"
#endif
#include <QMap>

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_record.h"
#include "contour_tracer.h"
#include "boundary_data.h"
#include "boundary_union_1d.h"

namespace qftbx {

/**
 * @brief Computes the QFT boundaries of a plant on the Nichols plane.
 *
 * For every design frequency \f$\omega\f$ and every grid point
 * \f$L = m\,e^{j\theta}\f$ of the Nichols window, the engine sweeps the
 * template \f$\{P\}\f$ and evaluates the closed-loop magnitudes (in dB)
 *
 * \f[
 *   D_{stab}   = \max_P \left|\frac{L}{P_0/P + L}\right|, \quad
 *   D_{track}  = \max_P |T| - \min_P |T|, \quad
 *   D_{out}    = \max_P \left|\frac{P_0/P}{P_0/P + L}\right|, \quad
 *   D_{in}     = \max_P \left|\frac{P_0}{P_0/P + L}\right|, \quad
 *   D_{ce}     = \max_P \left|\frac{L/P}{P_0/P + L}\right|
 * \f]
 *
 * building one sheet per specification family. Each sheet is then cut at
 * its specification bound (ContourTracer): the level curves are the
 * boundaries, each labelled with the side of the allowed region, and the 1D
 * union (BoundaryUnion1D) merges all specifications into the worst-case
 * boundary per frequency.
 *
 * Reference: I. Martinez Forte, PFC (documentos/pfc), boundary computation
 * chapter (sheet construction, contour cut and 1D union).
 */
class BoundaryEngine
{
public:

    BoundaryEngine();

    ~BoundaryEngine();

    BoundaryEngine(const BoundaryEngine &) = delete;
    BoundaryEngine & operator=(const BoundaryEngine &) = delete;

    /**
    * @brief Computes the boundaries of every design frequency.
    *
    * @param omega design frequencies (rad/s); the vector stays owned by the caller.
    * @param plant nominal plant \f$P_0\f$.
    * @param templates one value set per design frequency (full cloud or contour).
    * @param specifications the seven historical specification records; validated
    *        on entry (throws qftbx::InvalidInput on invalid used records).
    * @param phaseRange, phaseCount Nichols window phase axis (degrees).
    * @param magnitudeRange, magnitudeCount Nichols window magnitude axis (dB).
    * @param exportInfinity finite stand-in for infinity when the results are
    * EXPORTED (thesis ch. 7: a compatibility value for formats that cannot
    * carry an infinity); < 0 means "none given". It never takes part in the
    * sweep, which is IEEE throughout.
    *        (currently unused - see the (-180, 0 dB) decision, deferred).
    * @param cuda compute the sheets on the GPU (CUDA builds only).
    */
    void compute(QVector <qreal> * omega, LtiSystem * plant, const CloudSet & templates,
                 QVector <qftbx::SpecificationRecord *> * specifications, QPointF phaseRange,
                 qint32 phaseCount, QPointF magnitudeRange, qint32 magnitudeCount, qreal exportInfinity, bool cuda);

    /// A fresh non-owning view over the last computed results.
    BoundaryData * boundaryData();

    QVector <qreal> * omega();


private:
    SpecificationSet m_specifications;

    //Deep-frees the previous run's results (the engine owns them; the
    //BoundaryData views handed out never do).
    void releaseResults();

    void computeFrequencies(QVector <qreal> * omega, LtiSystem * plant, const CloudSet & templates,
                            QPointF phaseRange, qint32 phaseCount, QPointF magnitudeRange, qint32 magnitudeCount);

    void computeFrequency(qreal omega, LtiSystem * plant,
                          const ComplexCloud & valueSet, QVector <qreal> * phases,
                          QVector <qreal> * magnitudes, qint32 index);

    void traceFrequency(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound, QVector<QVector<QVector<qreal> *> *> *sheets,
                        QMap<QString, QVector<QPoint> *> *traceMetadata, std::complex<qreal> p0, const ComplexCloud & valueSet,
                        qint32 index, qreal phaseSpan, qreal magnitudeSpan, qreal phaseBottom, qreal magnitudeBottom);

    QVector<QVector<QPointF> *> *traceBoundary(qreal thresholdDb, QVector<QVector<qreal> *> *sheet,
                                               QVector<QPoint> *traceMetadata, std::complex<qreal> p0, const ComplexCloud & valueSet,
                                               qint32 kind, qreal phaseSpan, qreal magnitudeSpan,
                                               qreal phaseBottom, qreal magnitudeBottom);

    qint32 allowedZone(QVector <QPointF> * trace, std::complex<qreal> p0, const ComplexCloud & valueSet, qint32 kind, qreal thresholdDb);

#ifdef CUDA_AVAILABLE
    void traceFrequency(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound,
                        const BoundarySheetsCuda & cudaSheets,
                        QMap <QString, QVector <QPoint> * > * traceMetadata,
                        std::complex <qreal> p0, const ComplexCloud & valueSet, qint32 index,
                        qreal phaseSpan, qreal magnitudeSpan, qreal phaseBottom, qreal magnitudeBottom);

    QVector<QVector<QPointF> *> *traceBoundary(qreal thresholdDb, const float * sheet,
                                               QVector<QPoint> *traceMetadata, std::complex<qreal> p0, const ComplexCloud & valueSet,
                                               qint32 kind, qreal phaseSpan, qreal magnitudeSpan,
                                               qreal phaseBottom, qreal magnitudeBottom);
#endif


    QPointF m_phaseRange;
    QPointF m_magnitudeRange;
    qint32 m_phaseCount;
    qint32 m_magnitudeCount;


    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * m_boundaries;
    QVector <QMap <QString, QVector <QPoint> * > *> * m_traceMetadata;
    QVector <QVector <QPointF> * > * m_unionVectors;
    QVector< QVector< QVector<QPointF> * > * > * m_unionBuckets;

    QVector <bool> m_trackingMask;
    QVector <bool> m_stabilityMask;
    QVector <bool> m_noiseMask;
    QVector <bool> m_outputDisturbanceMask;
    QVector <bool> m_inputDisturbanceMask;
    QVector <bool> m_controlEffortMask;

    QVector <bool> * m_openFlags;
    QVector <bool> * m_upperFlags;

    //Alias of the caller's frequency vector: never freed here.
    QVector <qreal> * m_omega;

    bool m_cuda;

};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::BoundaryEngine;

#endif // QFTBX_BOUNDARY_ENGINE_H
