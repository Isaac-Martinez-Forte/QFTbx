/**
 * @file
 * @brief The boundary computation, frequency by frequency.
 *
 * Inputs are validated at the entry point: a specification with a null or
 * negative height, an inverted tracking band, a null plant or a single-point
 * axis throws here instead of degenerating the cut. The tracking band is
 * governed by its lower bound; the upper one only sets the cut height. The
 * sheets are column major, phase index times magnitude count plus magnitude
 * index, and the allowed side of each trace is probed one decibel below its
 * maximum against the same cut. Frequencies run in parallel and each writes
 * at its own index. The keys of the sheets are the ones the project file
 * stores. A CUDA twin of the tracing exists and is compiled only with CUDA.
 */

#include <chrono>
#include "src/core/math/constants.h"
#include <string>
#include <algorithm>
#include <vector>
#include <cstdint>
#include "src/core/common/record.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/boundaries/closed_form_columns.h"
#include "src/core/boundaries/closed_loop_worst_case.h"
#include "src/core/boundaries/singular_locus.h"

#include <iostream>

#include "src/core/math/sequence_vectors.h"

using std::complex;
using std::numeric_limits;

namespace qftbx {

void BoundaryEngine::releaseResults()
{
    m_boundaries.clear();
    m_traceMetadata.clear();
    m_columns.clear();
    m_unionVectors.clear();
    m_unionBuckets.clear();
    m_openFlags.clear();
    m_upperFlags.clear();
}

void BoundaryEngine::compute(std::vector<double> *omega, LtiSystem *plant, const CloudSet & templates,
                             bool templatesAreContours,
                             const qftbx::SpecificationRecords * specifications, qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange,
                             std::int32_t magnitudeCount, double exportInfinity, bool cuda){

    m_templatesAreContours = templatesAreContours;

    (void) exportInfinity;

    m_specifications = qftbx::toSpecificationSet(*specifications);
    m_cuda = cuda;

    if (phaseCount < 2 || magnitudeCount < 2) {
        throw InvalidInput(QFTBX_TR("Core", "The Nichols grid needs at least two points on each axis."));
    }
    if (!(phaseRange.width() > 0.0) || !(magnitudeRange.width() > 0.0)) {
        throw InvalidInput(QFTBX_TR("Core", "The Nichols grid needs a non-empty phase range and magnitude range."));
    }

    m_phaseCount = phaseCount;
    m_magnitudeCount = magnitudeCount;
    m_phaseRange = phaseRange;
    m_magnitudeRange = magnitudeRange;

    releaseResults();

    m_trackingMask.clear();
    m_stabilityMask.clear();
    m_noiseMask.clear();
    m_outputDisturbanceMask.clear();
    m_inputDisturbanceMask.clear();
    m_controlEffortMask.clear();

    for (double o : *omega){
        m_trackingMask.push_back(m_specifications.at(SpecificationType::TrackingLower).appliesAt(o));
        m_stabilityMask.push_back(m_specifications.at(SpecificationType::Stability).appliesAt(o));
        m_noiseMask.push_back(m_specifications.at(SpecificationType::SensorNoise).appliesAt(o));
        m_outputDisturbanceMask.push_back(m_specifications.at(SpecificationType::OutputDisturbance).appliesAt(o));
        m_inputDisturbanceMask.push_back(m_specifications.at(SpecificationType::InputDisturbance).appliesAt(o));
        m_controlEffortMask.push_back(m_specifications.at(SpecificationType::ControlEffort).appliesAt(o));
    }

    if (std::find(m_trackingMask.begin(), m_trackingMask.end(), true) != m_trackingMask.end() &&
            !m_specifications.at(SpecificationType::TrackingUpper).used()){
        throw InvalidInput(QFTBX_TR("Core", "The tracking boundary needs both tracking "
                           "specifications (T_L and T_U)."));
    }

#ifdef CUDA_AVAILABLE

    auto timer = std::chrono::steady_clock::now();

    if (!cuda){

        computeFrequencies(omega, plant, templates, phaseRange,
                           phaseCount, magnitudeRange, magnitudeCount);

        qftbx::record::write("boundaries", "sheets (OpenMP)",
                      qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                   std::chrono::steady_clock::now() - timer).count()));

    } else {

        m_boundaries.clear();
        m_traceMetadata.clear();
        m_columns.clear();

        for (std::size_t i = 0; i < omega->size(); i++){

            std::complex <double> p0 = plant->evaluate(omega->at(i));
            const ComplexCloud & valueSet = templates.at(i);

            const BoundarySheetsCuda cudaSheets = boundarySheetsCuda(
                valueSet, p0,
                qftbx::linspace1(phaseRange.min, phaseRange.max, phaseCount),
                qftbx::linspace1(magnitudeRange.min, magnitudeRange.max, magnitudeCount));

            std::map<std::string, TraceSet> bound;

            std::map<std::string, TraceLabels> traceMetadata;
            std::map<std::string, BoundaryColumns> columns;

            traceFrequency(omega->at(i), bound, cudaSheets, traceMetadata, columns, p0, valueSet, i,
                           phaseRange.width(), magnitudeRange.width(),
                           phaseRange.min, magnitudeRange.min);

            m_traceMetadata.push_back(std::move(traceMetadata));
            m_columns.push_back(std::move(columns));
            m_boundaries.push_back(std::move(bound));
        }

        qftbx::record::write("boundaries", "sheets (CUDA)",
                      qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                   std::chrono::steady_clock::now() - timer).count()));

    }
#else
    auto timer = std::chrono::steady_clock::now();

    computeFrequencies(omega, plant, templates, phaseRange, phaseCount, magnitudeRange, magnitudeCount);

    qftbx::record::write("boundaries", "sheets (OpenMP)",
                      qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                   std::chrono::steady_clock::now() - timer).count())
                          + " " + qftbx::record::number("frequencies", m_boundaries.size()));
#endif

    BoundaryUnion1D boundaryUnion;

    timer = std::chrono::steady_clock::now();

    {
        const BoundaryData view = boundaryData();
        boundaryUnion.run(view, m_traceMetadata);
    }

    qftbx::record::write("boundaries", "union",
                      qftbx::record::milliseconds(std::chrono::duration<double, std::milli>(
                                                   std::chrono::steady_clock::now() - timer).count()));

    m_unionVectors = boundaryUnion.takeUnionVectors();
    m_unionBuckets = boundaryUnion.takeUnionBuckets();

    m_openFlags = boundaryUnion.takeOpenFlags();
    m_upperFlags = boundaryUnion.takeUpperFlags();

    m_traceMetadata.clear();
}

BoundaryData BoundaryEngine::boundaryData(){
    return BoundaryData(m_boundaries, m_openFlags, m_upperFlags, m_phaseCount, m_phaseRange,
                        m_unionVectors, m_unionBuckets, m_magnitudeCount, m_magnitudeRange,
                        m_columns);
}

void BoundaryEngine::traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                                    const BoundarySheets & sheets,
                                    std::map<std::string, TraceLabels> & traceMetadata,
                                    std::map<std::string, BoundaryColumns> & columns,
                                    complex <double> p0, const ComplexCloud & valueSet, std::size_t index,
                                    double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom){

    if (m_trackingMask.at(index)){

        TraceLabels & metadata = traceMetadata["Tracking"];

        bound["Tracking"] =
                      traceBoundary(m_specifications.trackingSpreadDb(omega), sheets.at(1),
                                    metadata, p0, valueSet, 1, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["Tracking"] = sheetColumns(sheets.at(1), m_specifications.trackingSpreadDb(omega));
    }

    if (m_stabilityMask.at(index)){

        TraceLabels & metadata = traceMetadata["Stability"];

        bound["Stability"] =
                      traceBoundary(m_specifications.at(SpecificationType::Stability).boundDb(omega), sheets.at(0),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["Stability"] = sheetColumns(sheets.at(0), m_specifications.at(SpecificationType::Stability).boundDb(omega));
    }

    if (m_noiseMask.at(index)){

        TraceLabels & metadata = traceMetadata["SensorNoise"];

        bound["SensorNoise"] =
                      traceBoundary(m_specifications.at(SpecificationType::SensorNoise).boundDb(omega), sheets.at(0),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["SensorNoise"] = sheetColumns(sheets.at(0), m_specifications.at(SpecificationType::SensorNoise).boundDb(omega));
    }

    if (m_outputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["OutputDisturbance"];

        bound["OutputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega), sheets.at(2),
                                    metadata, p0, valueSet, 2, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["OutputDisturbance"] = sheetColumns(sheets.at(2), m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega));
    }

    if (m_inputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["InputDisturbance"];

        bound["InputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega), sheets.at(3),
                                    metadata, p0, valueSet, 3, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["InputDisturbance"] = sheetColumns(sheets.at(3), m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega));
    }

    if (m_controlEffortMask.at(index)){

        TraceLabels & metadata = traceMetadata["ControlEffort"];

        bound["ControlEffort"] =
                      traceBoundary(m_specifications.at(SpecificationType::ControlEffort).boundDb(omega), sheets.at(4),
                                    metadata, p0, valueSet, 4, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["ControlEffort"] = sheetColumns(sheets.at(4), m_specifications.at(SpecificationType::ControlEffort).boundDb(omega));
    }
}

#ifdef CUDA_AVAILABLE
void BoundaryEngine::traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                                    const BoundarySheetsCuda & cudaSheets,
                                    std::map<std::string, TraceLabels> & traceMetadata,
                                    std::map<std::string, BoundaryColumns> & columns,
                                    complex <double> p0, const ComplexCloud & valueSet, std::size_t index,
                                    double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom){

    if (m_trackingMask.at(index)){

        TraceLabels & metadata = traceMetadata["Tracking"];

        bound["Tracking"] =
                      traceBoundary(m_specifications.trackingSpreadDb(omega), cudaSheets.tracking.data(),
                                    metadata, p0, valueSet, 1, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["Tracking"] = sheetColumns(cudaSheets.tracking.data(), m_specifications.trackingSpreadDb(omega));
    }

    if (m_stabilityMask.at(index)){

        TraceLabels & metadata = traceMetadata["Stability"];

        bound["Stability"] =
                      traceBoundary(m_specifications.at(SpecificationType::Stability).boundDb(omega), cudaSheets.stabilityNoise.data(),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["Stability"] = sheetColumns(cudaSheets.stabilityNoise.data(), m_specifications.at(SpecificationType::Stability).boundDb(omega));
    }

    if (m_noiseMask.at(index)){

        TraceLabels & metadata = traceMetadata["SensorNoise"];

        bound["SensorNoise"] =
                      traceBoundary(m_specifications.at(SpecificationType::SensorNoise).boundDb(omega), cudaSheets.stabilityNoise.data(),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["SensorNoise"] = sheetColumns(cudaSheets.stabilityNoise.data(), m_specifications.at(SpecificationType::SensorNoise).boundDb(omega));
    }

    if (m_outputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["OutputDisturbance"];

        bound["OutputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega), cudaSheets.outputDisturbance.data(),
                                    metadata, p0, valueSet, 2, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["OutputDisturbance"] = sheetColumns(cudaSheets.outputDisturbance.data(), m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega));
    }

    if (m_inputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["InputDisturbance"];

        bound["InputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega), cudaSheets.inputDisturbance.data(),
                                    metadata, p0, valueSet, 3, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["InputDisturbance"] = sheetColumns(cudaSheets.inputDisturbance.data(), m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega));
    }

    if (m_controlEffortMask.at(index)){

        TraceLabels & metadata = traceMetadata["ControlEffort"];

        bound["ControlEffort"] =
                      traceBoundary(m_specifications.at(SpecificationType::ControlEffort).boundDb(omega), cudaSheets.controlEffort.data(),
                                    metadata, p0, valueSet, 4, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
        columns["ControlEffort"] = sheetColumns(cudaSheets.controlEffort.data(), m_specifications.at(SpecificationType::ControlEffort).boundDb(omega));
    }

}
#endif

BoundaryColumns BoundaryEngine::sheetColumns(const BoundarySheet & sheet, double thresholdDb) const
{
    const auto cell = [&sheet](std::int32_t phase, std::int32_t magnitude) {
        return sheet[static_cast<std::size_t>(magnitude)][static_cast<std::size_t>(phase)];
    };
    return BoundaryColumns::fromSheet(cell, thresholdDb, m_phaseCount, m_phaseRange, m_magnitudeCount, m_magnitudeRange);
}

#ifdef CUDA_AVAILABLE
BoundaryColumns BoundaryEngine::sheetColumns(const float * sheet, double thresholdDb) const
{
    const std::size_t height = static_cast<std::size_t>(m_magnitudeCount);
    const auto cell = [sheet, height](std::int32_t phase, std::int32_t magnitude) {
        return sheet[static_cast<std::size_t>(phase) * height + static_cast<std::size_t>(magnitude)];
    };
    return BoundaryColumns::fromSheet(cell, thresholdDb, m_phaseCount, m_phaseRange, m_magnitudeCount, m_magnitudeRange);
}
#endif

TraceSet BoundaryEngine::traceBoundary(double thresholdDb, const BoundarySheet & sheet,
                                       TraceLabels & traceMetadata, std::complex<double> p0,
                                       const ComplexCloud & valueSet,
                                       std::int32_t kind, double phaseSpan, double magnitudeSpan,
                                       double phaseBottom, double magnitudeBottom)
{

    ContourTracer tracer (thresholdDb, sheet);

    TraceSet traces = tracer.trace(phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);

    std::vector<char> allowed(traces.size(), 0);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t j = 0; j < traces.size(); ++j) {
        allowed[j] = allowedZone(traces.at(j), p0, valueSet, kind, thresholdDb) != 0;
    }
    traceMetadata.assign(allowed.begin(), allowed.end());

    if (traceMetadata.empty()) {
        traceMetadata.push_back(false);
    }

    return traces;
}

#ifdef CUDA_AVAILABLE
TraceSet BoundaryEngine::traceBoundary(double thresholdDb, const float *sheet,
                                       TraceLabels & traceMetadata, std::complex<double> p0,
                                       const ComplexCloud & valueSet, std::int32_t kind,
                                       double phaseSpan, double magnitudeSpan,
                                       double phaseBottom, double magnitudeBottom){

    ContourTracer tracer (thresholdDb, sheet);

    TraceSet traces = tracer.trace(phaseSpan, m_phaseCount, magnitudeSpan,
                                   m_magnitudeCount, phaseBottom, magnitudeBottom);

    std::vector<char> allowed(traces.size(), 0);

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t j = 0; j < traces.size(); ++j) {
        allowed[j] = allowedZone(traces.at(j), p0, valueSet, kind, thresholdDb) != 0;
    }
    traceMetadata.assign(allowed.begin(), allowed.end());

    return traces;
}
#endif

namespace {

std::complex<double> nicholsToComplex(double magnitudeDb, double phaseDegrees)
{
    const double linearMagnitude = std::pow(10.0, magnitudeDb / 20.0);
    return std::polar(linearMagnitude, phaseDegrees * qftbx::math::kPi / 180.0);
}

}

std::int32_t BoundaryEngine::allowedZone(const Trace & trace, complex <double> p0, const ComplexCloud & valueSet,
                                   std::int32_t kind, double thresholdDb){

    double probeMagnitude = -numeric_limits<double>::infinity();
    double probePhase = -numeric_limits<double>::infinity();

    for (const qftbx::NicholsPoint & point : trace) {
        if(point.magnitude > probeMagnitude){
            probeMagnitude = point.magnitude;
            probePhase = point.phase;
        }
    }

    probeMagnitude -= 1;

    const complex<double> L = nicholsToComplex(probeMagnitude, probePhase);
    const WorstCase worst = worstCaseAt(p0, L, valueSet, nominalOverValueSet(p0, valueSet));

    switch (kind){
    case 0:
        if (20 * log10(worst.stabilityNoise) > thresholdDb){
            return 0;
        }
        break;
    case 1:
        if ((20 * log10(worst.stabilityNoise) - 20 * log10(worst.trackingMin)) > thresholdDb){
            return 0;
        }
        break;
    case 2:
        if(20 * log10(worst.outputDisturbance) > thresholdDb){
            return 0;
        }
        break;
    case 3:
        if (20 * log10(worst.inputDisturbance) > thresholdDb){
            return 0;
        }
        break;
    case 4:
        if (20 * log10(worst.controlEffort) > thresholdDb){
            return 0;
        }
        break;
    default:
        return 1;
    }

    return 1;
}

void BoundaryEngine::computeFrequencies(std::vector<double> *omega, LtiSystem *plant,
                                        const CloudSet & templates, qftbx::Range phaseRange, std::int32_t phaseCount,
                                        qftbx::Range magnitudeRange, std::int32_t magnitudeCount)
{
    const std::vector <double> phases = qftbx::linspace(phaseRange.min, phaseRange.max, phaseCount);
    const std::vector <double> magnitudes = qftbx::linspace(magnitudeRange.min, magnitudeRange.max,
                                                      magnitudeCount);

    m_boundaries.assign(static_cast<std::size_t>(omega->size()), {});
    m_traceMetadata.assign(static_cast<std::size_t>(omega->size()), {});
    m_columns.assign(static_cast<std::size_t>(omega->size()), {});

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t i = 0; i < omega->size(); ++i){

        if (cancellationAsked(m_cancellation)) {
            continue;
        }

        computeFrequency(omega->at(i), plant, templates.at(i), phases, magnitudes, i);
    }

    if (cancellationAsked(m_cancellation)) {
        m_boundaries.clear();
        m_traceMetadata.clear();
        m_columns.clear();
        throw qftbx::Cancelled();
    }

}

namespace {

double violatingDb(double valueDb)
{
    if (std::isnan(valueDb)) {
        return std::numeric_limits<double>::infinity();
    }

    return valueDb;
}

}

void BoundaryEngine::computeFrequency (double omega, LtiSystem * plant,
                                       const ComplexCloud & valueSet,
                                       const std::vector <double> & phases,
                                       const std::vector <double> & magnitudes, std::size_t index){

    complex <double> p0 = plant->evaluate(omega);

    const ComplexCloud & p = valueSet;

    BoundarySheets sheets;

    BoundarySheet & stabilityNoiseSheet = sheets.at(0);
    BoundarySheet & trackingSheet = sheets.at(1);
    BoundarySheet & outputDisturbanceSheet = sheets.at(2);
    BoundarySheet & inputDisturbanceSheet = sheets.at(3);
    BoundarySheet & controlEffortSheet = sheets.at(4);

    const std::size_t rowCount = static_cast<std::size_t>(magnitudes.size());
    for (BoundarySheet * sheet : {&stabilityNoiseSheet, &trackingSheet, &outputDisturbanceSheet,
                                 &inputDisturbanceSheet, &controlEffortSheet}) {
        sheet->reserve(rowCount);
    }

    const std::vector<complex<double>> nominalOverP = nominalOverValueSet(p0, p);

    WorstCaseMask mask;
    mask.stabilityNoiseTracking = m_stabilityMask.at(index) || m_noiseMask.at(index) || m_trackingMask.at(index);
    mask.outputDisturbance = m_outputDisturbanceMask.at(index);
    mask.inputDisturbance = m_inputDisturbanceMask.at(index);
    mask.controlEffort = m_controlEffortMask.at(index);

    const bool needStabilityNoise = m_stabilityMask.at(index) || m_noiseMask.at(index);
    const bool needTracking = m_trackingMask.at(index);

    const SingularLocus locus(nominalOverP, m_templatesAreContours);

    for (std::size_t k = 0; k < magnitudes.size(); ++k){

        std::vector<double> stabilityNoiseRow;
        std::vector<double> trackingRow;
        std::vector<double> outputDisturbanceRow;
        std::vector<double> inputDisturbanceRow;
        std::vector<double> controlEffortRow;

        const std::size_t rowWidth = static_cast<std::size_t>(phases.size());
        stabilityNoiseRow.reserve(rowWidth);
        trackingRow.reserve(rowWidth);
        outputDisturbanceRow.reserve(rowWidth);
        inputDisturbanceRow.reserve(rowWidth);
        controlEffortRow.reserve(rowWidth);

        for (std::size_t j = 0; j < phases.size(); ++j){
            const complex<double> L = nicholsToComplex(magnitudes[k], phases[j]);

            const WorstCase sampled = worstCaseAt(p0, L, p, nominalOverP, mask);
            const WorstCase worst = m_guardSingularLocus ? locus.guard(sampled, L, p0) : sampled;

            if (needStabilityNoise) {
                stabilityNoiseRow.push_back(violatingDb(20 * log10(worst.stabilityNoise)));
            }
            if (needTracking) {
                trackingRow.push_back(violatingDb((20 * log10(worst.stabilityNoise)) - (20 * log10(worst.trackingMin))));
            }
            if (mask.outputDisturbance) {
                outputDisturbanceRow.push_back(violatingDb(20 * log10(worst.outputDisturbance)));
            }
            if (mask.inputDisturbance) {
                inputDisturbanceRow.push_back(violatingDb(20 * log10(worst.inputDisturbance)));
            }
            if (mask.controlEffort) {
                controlEffortRow.push_back(violatingDb(20 * log10(worst.controlEffort)));
            }
        }
        if (needStabilityNoise) stabilityNoiseSheet.push_back(std::move(stabilityNoiseRow));
        if (needTracking) trackingSheet.push_back(std::move(trackingRow));
        if (mask.outputDisturbance) outputDisturbanceSheet.push_back(std::move(outputDisturbanceRow));
        if (mask.inputDisturbance) inputDisturbanceSheet.push_back(std::move(inputDisturbanceRow));
        if (mask.controlEffort) controlEffortSheet.push_back(std::move(controlEffortRow));
    }

    std::map<std::string, TraceSet> bound;

    std::map<std::string, TraceLabels> traceMetadata;
    std::map<std::string, BoundaryColumns> columns;

    traceFrequency(omega, bound, sheets, traceMetadata, columns, p0, p, index,
                   m_phaseRange.width(), m_magnitudeRange.width(),
                   m_phaseRange.min, m_magnitudeRange.min);

    if (m_closedFormColumns && m_templatesAreContours) {
        const SingularLocus * guard = m_guardSingularLocus ? &locus : nullptr;
        const auto replace = [&](const char * name, SpecificationType type, bool used) {
            if (used) {
                columns[name] = ClosedFormColumns::columns(type, m_specifications.at(type).boundDb(omega),
                                                          p0, p, nominalOverP, phases, m_phaseRange, guard);
            }
        };
        replace("Stability", SpecificationType::Stability, m_stabilityMask.at(index));
        replace("SensorNoise", SpecificationType::SensorNoise, m_noiseMask.at(index));
        replace("OutputDisturbance", SpecificationType::OutputDisturbance, m_outputDisturbanceMask.at(index));
        replace("InputDisturbance", SpecificationType::InputDisturbance, m_inputDisturbanceMask.at(index));
        replace("ControlEffort", SpecificationType::ControlEffort, m_controlEffortMask.at(index));
    }

    m_traceMetadata[index] = std::move(traceMetadata);
    m_columns[index] = std::move(columns);
    m_boundaries[index] = std::move(bound);
}

}
