#ifndef QFTBX_BOUNDARY_ENGINE_H
#define QFTBX_BOUNDARY_ENGINE_H

#include <cstdint>
#include <QVector>

#include "src/core/templates/cloud_set.h"
#include "src/core/boundaries/boundary_types.h"
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
    void compute(QVector <double> * omega, LtiSystem * plant, const CloudSet & templates,
                 const qftbx::SpecificationRecords * specifications, QPointF phaseRange,
                 std::int32_t phaseCount, QPointF magnitudeRange, std::int32_t magnitudeCount, double exportInfinity, bool cuda);

    /// A fresh non-owning view over the last computed results.
    /// A snapshot of the results, by value. It used to be a freshly
    /// allocated NON-OWNING view that every caller had to delete and that
    /// nothing in the type said was a view.
    BoundaryData boundaryData();

    QVector <double> * omega();


private:
    SpecificationSet m_specifications;

    //Deep-frees the previous run's results (the engine owns them; the
    //BoundaryData views handed out never do).
    void releaseResults();

    void computeFrequencies(QVector <double> * omega, LtiSystem * plant, const CloudSet & templates,
                            QPointF phaseRange, std::int32_t phaseCount, QPointF magnitudeRange, std::int32_t magnitudeCount);

    void computeFrequency(double omega, LtiSystem * plant,
                          const ComplexCloud & valueSet, const QVector <double> & phases,
                          const QVector <double> & magnitudes, std::int32_t index);

    void traceFrequency(double omega, std::map<QString, TraceSet> & bound,
                        const BoundarySheets & sheets,
                        std::map<QString, TraceLabels> & traceMetadata, std::complex<double> p0, const ComplexCloud & valueSet,
                        std::int32_t index, double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom);

    TraceSet traceBoundary(double thresholdDb, const BoundarySheet & sheet,
                                               TraceLabels & traceMetadata, std::complex<double> p0, const ComplexCloud & valueSet,
                                               std::int32_t kind, double phaseSpan, double magnitudeSpan,
                                               double phaseBottom, double magnitudeBottom);

    std::int32_t allowedZone(const Trace & trace, std::complex<double> p0, const ComplexCloud & valueSet, std::int32_t kind, double thresholdDb);

#ifdef CUDA_AVAILABLE
    void traceFrequency(double omega, std::map<QString, TraceSet> & bound,
                        const BoundarySheetsCuda & cudaSheets,
                        std::map<QString, TraceLabels> & traceMetadata,
                        std::complex <double> p0, const ComplexCloud & valueSet, std::int32_t index,
                        double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom);

    TraceSet traceBoundary(double thresholdDb, const float * sheet,
                                               QVector<QPoint> *traceMetadata, std::complex<double> p0, const ComplexCloud & valueSet,
                                               std::int32_t kind, double phaseSpan, double magnitudeSpan,
                                               double phaseBottom, double magnitudeBottom);
#endif


    QPointF m_phaseRange;
    QPointF m_magnitudeRange;
    std::int32_t m_phaseCount = 0;
    std::int32_t m_magnitudeCount = 0;


    BoundarySet m_boundaries;
    TraceMetadata m_traceMetadata;
    UnionTraces m_unionVectors;
    UnionBuckets m_unionBuckets;

    QVector <bool> m_trackingMask;
    QVector <bool> m_stabilityMask;
    QVector <bool> m_noiseMask;
    QVector <bool> m_outputDisturbanceMask;
    QVector <bool> m_inputDisturbanceMask;
    QVector <bool> m_controlEffortMask;

    std::vector<bool> m_openFlags;
    std::vector<bool> m_upperFlags;

    //Alias of the caller's frequency vector: never freed here.
    QVector <double> * m_omega = nullptr;

    bool m_cuda = false;

};

} // namespace qftbx

//Transitional: consumers still refer to the class unqualified.
using qftbx::BoundaryEngine;

#endif // QFTBX_BOUNDARY_ENGINE_H
