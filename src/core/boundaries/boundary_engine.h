#ifndef QFTBX_BOUNDARY_ENGINE_H
#define QFTBX_BOUNDARY_ENGINE_H

#include <string>
#include <cstdint>
#include "src/core/math/range.h"
#include <vector>

#include "src/core/templates/cloud_set.h"
#include "src/core/boundaries/boundary_types.h"
#include <complex>
#include <limits>
#include <cmath>
#ifdef CUDA_AVAILABLE
#include "src/core/gpu/boundary_sheets_cuda.h"
#endif

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/boundaries/contour_tracer.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/boundary_union_1d.h"

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
 * boundary per frequency. The same cut read column by column gives the
 * allowed magnitude intervals the search classifies against
 * (BoundaryColumns).
 *
 * Reference: I. Martinez Forte, PFC (documentos/pfc), boundary computation
 * chapter (sheet construction, contour cut and 1D union).
 */
class BoundaryEngine
{
public:
    BoundaryEngine() = default;
    BoundaryEngine(const BoundaryEngine &) = delete;
    BoundaryEngine & operator=(const BoundaryEngine &) = delete;

    /**
    * @brief Computes the boundaries of every design frequency.
    *
    * @param omega design frequencies (rad/s); the vector stays owned by the caller.
    * @param plant nominal plant \f$P_0\f$.
    * @param templates one value set per design frequency (full cloud or contour).
    * @param templatesAreContours whether 'templates' are the epsilon-hull
    *        contours (ordered closed walks, one per frequency) or the full
    *        clouds. Near the singular locus the sweep guards its sample
    *        differently for each; see SingularLocus. The CUDA path does not
    *        apply that guard yet.
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
    void compute(std::vector <double> * omega, LtiSystem * plant, const CloudSet & templates,
                 bool templatesAreContours,
                 const qftbx::SpecificationRecords * specifications, qftbx::Range phaseRange,
                 std::int32_t phaseCount, qftbx::Range magnitudeRange, std::int32_t magnitudeCount, double exportInfinity, bool cuda);

    /// Whether the sweep guards its sample near the singular locus (see
    /// SingularLocus): on by default, which is step 2 of Moreno, Banos and
    /// Berenguel's algorithm 2.1 with the border between samples covered as
    /// well. Off reproduces the raw sweep over the sample, which is what the
    /// legacy boundaries were computed with.
    void setSingularLocusGuard(bool on) { m_guardSingularLocus = on; }
    bool singularLocusGuard() const { return m_guardSingularLocus; }

    /// Read the columns of the five magnitude specifications in closed form
    /// (ClosedFormColumns) instead of off their sheets: exact in magnitude,
    /// no window. Tracking keeps its sheet; the sheets are still computed
    /// for the traced curves. Off by default.
    void setClosedFormColumns(bool on) { m_closedFormColumns = on; }
    bool closedFormColumns() const { return m_closedFormColumns; }

    /// A snapshot of the results, by value. It used to be a freshly
    /// allocated NON-OWNING view that every caller had to delete and that
    /// nothing in the type said was a view.
    BoundaryData boundaryData();


private:
    SpecificationSet m_specifications;
    //Whether the value sets swept are the epsilon-hull contours (ordered
    //walks) or the full clouds: the guard near the singular locus reads
    //them differently (see SingularLocus).
    bool m_templatesAreContours = false;
    bool m_guardSingularLocus = true;
    bool m_closedFormColumns = false;

    //Clears the previous run's results.
    void releaseResults();

    void computeFrequencies(std::vector <double> * omega, LtiSystem * plant, const CloudSet & templates,
                            qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange, std::int32_t magnitudeCount);

    void computeFrequency(double omega, LtiSystem * plant,
                          const ComplexCloud & valueSet, const std::vector <double> & phases,
                          const std::vector <double> & magnitudes, std::size_t index);

    void traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                        const BoundarySheets & sheets,
                        std::map<std::string, TraceLabels> & traceMetadata,
                        std::map<std::string, BoundaryColumns> & columns,
                        std::complex<double> p0, const ComplexCloud & valueSet,
                        std::size_t index, double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom);

    //The allowed magnitude intervals per phase column of one specification,
    //read off its sheet at the cut height (BoundaryColumns::fromSheet).
    BoundaryColumns sheetColumns(const BoundarySheet & sheet, double thresholdDb) const;

    TraceSet traceBoundary(double thresholdDb, const BoundarySheet & sheet,
                                               TraceLabels & traceMetadata, std::complex<double> p0, const ComplexCloud & valueSet,
                                               std::int32_t kind, double phaseSpan, double magnitudeSpan,
                                               double phaseBottom, double magnitudeBottom);

    std::int32_t allowedZone(const Trace & trace, std::complex<double> p0, const ComplexCloud & valueSet, std::int32_t kind, double thresholdDb);

#ifdef CUDA_AVAILABLE
    void traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                        const BoundarySheetsCuda & cudaSheets,
                        std::map<std::string, TraceLabels> & traceMetadata,
                        std::map<std::string, BoundaryColumns> & columns,
                        std::complex <double> p0, const ComplexCloud & valueSet, std::size_t index,
                        double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom);

    BoundaryColumns sheetColumns(const float * sheet, double thresholdDb) const;

    TraceSet traceBoundary(double thresholdDb, const float * sheet,
                           TraceLabels & traceMetadata, std::complex<double> p0,
                           const ComplexCloud & valueSet, std::int32_t kind,
                           double phaseSpan, double magnitudeSpan,
                           double phaseBottom, double magnitudeBottom);
#endif


    qftbx::Range m_phaseRange;
    qftbx::Range m_magnitudeRange;
    std::int32_t m_phaseCount = 0;
    std::int32_t m_magnitudeCount = 0;


    BoundarySet m_boundaries;
    TraceMetadata m_traceMetadata;
    ColumnSet m_columns;
    UnionTraces m_unionVectors;
    UnionBuckets m_unionBuckets;

    std::vector <bool> m_trackingMask;
    std::vector <bool> m_stabilityMask;
    std::vector <bool> m_noiseMask;
    std::vector <bool> m_outputDisturbanceMask;
    std::vector <bool> m_inputDisturbanceMask;
    std::vector <bool> m_controlEffortMask;

    std::vector<bool> m_openFlags;
    std::vector<bool> m_upperFlags;

    bool m_cuda = false;

};

} // namespace qftbx


#endif // QFTBX_BOUNDARY_ENGINE_H
