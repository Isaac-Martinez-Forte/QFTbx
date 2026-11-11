/**
 * @file
 * @brief Computation of the QFT boundaries of a plant on the Nichols plane.
 *
 * For every design frequency \f$\omega\f$ and every grid point
 * \f$L = m\,e^{j\theta}\f$ of the Nichols window, the engine sweeps the
 * template \f$\{P\}\f$ and builds one sheet per specification family from
 * the closed-loop magnitudes in dB, with \f$P_0\f$ the nominal plant:
 *
 * \f[
 *   D_{stab}   = \max_P \left|\frac{L}{P_0/P + L}\right|, \quad
 *   D_{track}  = \max_P |T| - \min_P |T|, \quad
 *   D_{out}    = \max_P \left|\frac{P_0/P}{P_0/P + L}\right|, \quad
 *   D_{in}     = \max_P \left|\frac{P_0}{P_0/P + L}\right|, \quad
 *   D_{ce}     = \max_P \left|\frac{L/P}{P_0/P + L}\right|
 * \f]
 *
 * Each sheet is cut at its specification bound (ContourTracer); the level
 * curves are the boundaries, each labelled with its allowed side, and the
 * 1D union (BoundaryUnion1D) merges them into the worst-case boundary per
 * frequency. The same cut read column by column gives the allowed
 * magnitude intervals the search classifies against (BoundaryColumns).
 * Reference: I. Martínez Forte, final-year project, boundary computation
 * chapter.
 *
 * Frequencies are in rad/s, phases in degrees and magnitudes in dB; the
 * caller keeps the frequency vector, and invalid specification records
 * throw InvalidInput. The templates are the epsilon-hull contours or the
 * full clouds, which the guard near the singular locus (SingularLocus), on
 * by default, reads differently. Closed-form columns, off by default, read the five magnitude
 * specifications exactly instead of off their sheets, which the curves
 * still use; tracking keeps the sheet's columns, and so does a cloud,
 * whose guard has no closed form. The export infinity, negative when not
 * given, is a finite stand-in for formats that cannot carry one and never
 * enters the sweep, which is IEEE throughout. A cancellation flag is read
 * once per frequency; a cancelled computation throws Cancelled and keeps
 * nothing. boundaryData() is a snapshot by value. adoptSheets() takes the
 * sheets of every frequency computed elsewhere, on the GPU, and does the
 * rest: the traces, the columns and the union. adoptBoundaries() takes the
 * traces, their labels and the columns read elsewhere, and does the
 * closed-form columns, when they are on, and the union.
 */

#ifndef QFTBX_BOUNDARY_ENGINE_H
#define QFTBX_BOUNDARY_ENGINE_H

#include <string>
#include "src/core/pipeline/cancellation.h"
#include <cstdint>
#include "src/core/math/range.h"
#include <vector>

#include "src/core/templates/cloud_set.h"
#include "src/core/boundaries/boundary_types.h"
#include <complex>
#include <limits>
#include <cmath>

#include "src/core/system/lti_system.h"
#include "src/core/specifications/specification.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/boundaries/contour_tracer.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/boundaries/boundary_union_1d.h"
#include "src/core/boundaries/boundary_computation.h"
#include "src/core/boundaries/boundary_plan.h"

namespace qftbx {

class SingularLocus;

struct FrequencyBoundaries
{
    std::map<std::string, TraceSet> traces;
    std::map<std::string, TraceLabels> labels;
    std::map<std::string, BoundaryColumns> columns;
};

class BoundaryEngine : public BoundaryComputation
{
public:
    BoundaryEngine() = default;
    BoundaryEngine(const BoundaryEngine &) = delete;
    BoundaryEngine & operator=(const BoundaryEngine &) = delete;

    void compute(std::vector <double> * omega, LtiSystem * plant, const CloudSet & templates,
                 bool templatesAreContours,
                 const qftbx::SpecificationRecords * specifications, qftbx::Range phaseRange,
                 std::int32_t phaseCount, qftbx::Range magnitudeRange, std::int32_t magnitudeCount, double exportInfinity) override;

    void setCancellation(const qftbx::CancellationToken * token) override { m_cancellation = token; }

    void setSingularLocusGuard(bool on) override { m_guardSingularLocus = on; }
    bool singularLocusGuard() const { return m_guardSingularLocus; }

    void setClosedFormColumns(bool on) override { m_closedFormColumns = on; }
    bool closedFormColumns() const { return m_closedFormColumns; }

    BoundaryData boundaryData() override;

    void adoptSheets(std::vector<double> * omega, LtiSystem * plant, const CloudSet & templates,
                     bool templatesAreContours, BoundaryPlan plan,
                     qftbx::Range phaseRange, std::int32_t phaseCount,
                     qftbx::Range magnitudeRange, std::int32_t magnitudeCount,
                     std::vector<BoundarySheets> sheets);

    void adoptBoundaries(std::vector<double> * omega, LtiSystem * plant, const CloudSet & templates,
                         bool templatesAreContours, BoundaryPlan plan,
                         qftbx::Range phaseRange, std::int32_t phaseCount,
                         qftbx::Range magnitudeRange, std::int32_t magnitudeCount,
                         std::vector<FrequencyBoundaries> boundaries);

private:
    const qftbx::CancellationToken * m_cancellation = nullptr;

    SpecificationSet m_specifications;
    bool m_templatesAreContours = false;
    bool m_guardSingularLocus = true;
    bool m_closedFormColumns = false;

    void releaseResults();

    void adoptPlan(BoundaryPlan plan, bool templatesAreContours, qftbx::Range phaseRange, std::int32_t phaseCount,
                   qftbx::Range magnitudeRange, std::int32_t magnitudeCount);

    void uniteFrequencies();

    void computeFrequencies(std::vector <double> * omega, LtiSystem * plant, const CloudSet & templates,
                            qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange, std::int32_t magnitudeCount);

    void computeFrequency(double omega, LtiSystem * plant,
                          const ComplexCloud & valueSet, const std::vector <double> & phases,
                          const std::vector <double> & magnitudes, std::size_t index);

    BoundarySheets sheetsAt(std::complex<double> p0, const ComplexCloud & valueSet,
                            const std::vector<std::complex<double>> & nominalOverP, const SingularLocus & locus,
                            const std::vector <double> & phases, const std::vector <double> & magnitudes,
                            std::size_t index) const;

    void closedFormColumns(double omega, std::complex<double> p0, const ComplexCloud & valueSet,
                           const std::vector<std::complex<double>> & nominalOverP, const SingularLocus * locus,
                           const std::vector <double> & phases, std::size_t index,
                           std::map<std::string, BoundaryColumns> & columns) const;

    void readSheets(double omega, std::complex<double> p0, const ComplexCloud & valueSet,
                    const std::vector<std::complex<double>> & nominalOverP, const SingularLocus * locus,
                    const BoundarySheets & sheets, const std::vector <double> & phases, std::size_t index);

    void traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                        const BoundarySheets & sheets,
                        std::map<std::string, TraceLabels> & traceMetadata,
                        std::map<std::string, BoundaryColumns> & columns,
                        std::complex<double> p0, const ComplexCloud & valueSet,
                        std::size_t index, double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom);

    BoundaryColumns sheetColumns(const BoundarySheet & sheet, double thresholdDb) const;

    TraceSet traceBoundary(double thresholdDb, const BoundarySheet & sheet,
                                               TraceLabels & traceMetadata, std::complex<double> p0, const ComplexCloud & valueSet,
                                               std::int32_t kind, double phaseSpan, double magnitudeSpan,
                                               double phaseBottom, double magnitudeBottom);

    std::int32_t allowedZone(const Trace & trace, std::complex<double> p0, const ComplexCloud & valueSet, std::int32_t kind, double thresholdDb);

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


};

}

#endif
