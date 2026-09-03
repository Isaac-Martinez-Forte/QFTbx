#include <chrono>
#include <string>
#include <algorithm>
#include <vector>
#include <cstdint>
#include "boundary_engine.h"

#include "src/core/math/parser_warmup.h"

#include <iostream>

#include "src/core/math/sequence_vectors.h" //linspace for the sheet axes

using std::complex;
using std::cout;
using std::endl;
using std::numeric_limits;

namespace qftbx {

//The CUDA interface lives in src/core/gpu/boundary_sheets_cuda.h (plain
//C++ header; only the .cu needs nvcc).

BoundaryEngine::BoundaryEngine()
{
    m_omega = nullptr;
    m_cuda = false;
}

BoundaryEngine::~BoundaryEngine()
{
    releaseResults();
}

void BoundaryEngine::releaseResults()
{
    //Forty lines of nested hand-written deletion used to live here, five
    //levels deep, and every one of them had to agree with whoever else held
    //a pointer into the same structure. Clearing the containers is the whole
    //of it now.
    m_boundaries.clear();
    m_traceMetadata.clear();
    m_unionVectors.clear();
    m_unionBuckets.clear();
    m_openFlags.clear();
    m_upperFlags.clear();

    //m_omega aliases the caller's vector: never freed here.
    m_omega = nullptr;
}

void BoundaryEngine::compute(std::vector<double> *omega, LtiSystem *plant, const CloudSet & templates,
                             const qftbx::SpecificationRecords * specifications, qftbx::Range phaseRange, std::int32_t phaseCount, qftbx::Range magnitudeRange,
                             std::int32_t magnitudeCount, double exportInfinity, bool cuda){

    //The export stand-in for infinity is not part of the computation
    //(thesis ch. 7: it exists so exported data can carry a finite value in
    //formats that cannot represent an infinity). It is kept in the
    //interface for the numeric export, still to be implemented.
    (void) exportInfinity;



    //The historical records are validated on conversion: a used
    //specification with height <= 0, an inverted band or a null plant
    //throws qftbx::InvalidInput here, at the entry point, instead of
    //silently degenerating the cut.
    m_specifications = qftbx::toSpecificationSet(*specifications);
    m_cuda = cuda;

    m_phaseCount = phaseCount;
    m_magnitudeCount = magnitudeCount;
    m_phaseRange = phaseRange;
    m_magnitudeRange = magnitudeRange;

    //The previous run's results are freed here: every run used to pile new
    //containers on top of the old ones.
    releaseResults();

    m_trackingMask.clear();
    m_stabilityMask.clear();
    m_noiseMask.clear();
    m_outputDisturbanceMask.clear();
    m_inputDisturbanceMask.clear();
    m_controlEffortMask.clear();

    //As always, the tracking band is governed by T_L (the lower bound);
    //T_U only provides the cut height.
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
        //The historical code dereferenced T_U's null plant.
        throw InvalidInput("The tracking boundary needs both tracking "
                           "specifications (T_L and T_U).");
    }



#ifdef CUDA_AVAILABLE

    const auto timer = std::chrono::steady_clock::now();

    if (!cuda){

        computeFrequencies(omega, plant, templates, phaseRange,
                           phaseCount, magnitudeRange, magnitudeCount);

        cout << "boundaries OpenMP: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer).count() << " milliseconds" << endl;

    } else {


        m_boundaries.clear();
        m_traceMetadata.clear();

        for (int i = 0; i < omega->size(); i++){

            std::complex <double> p0 = plant->evaluate(omega->at(i));
            const ComplexCloud & valueSet = templates.at(i);

            //Sheets come back by value in one struct: the old raw float*
            //vector leaked all five sheets on every frequency.
            const BoundarySheetsCuda cudaSheets = boundarySheetsCuda(
                valueSet, p0,
                tools::linspace1(phaseRange.min, phaseRange.max, phaseCount),
                tools::linspace1(magnitudeRange.min, magnitudeRange.max, magnitudeCount));

            std::map<std::string, TraceSet> bound;

            std::map<std::string, TraceLabels> traceMetadata;

            traceFrequency(omega->at(i), bound, cudaSheets, traceMetadata, p0, valueSet, i,
                           phaseRange.magnitude - phaseRange.phase, magnitudeRange.magnitude - magnitudeRange.phase,
                           phaseRange.phase, magnitudeRange.phase);

            m_traceMetadata.push_back(std::move(traceMetadata));
            m_boundaries.push_back(std::move(bound));
        }

        //Alias of the caller's vector, like the CPU path (the old copy was
        //the one leak releaseResults could not free).
        m_omega = omega;

        cout << "boundaries CUDA: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer).count() << " milliseconds" << endl;


    }
#else
    auto timer = std::chrono::steady_clock::now();

    computeFrequencies(omega, plant, templates, phaseRange, phaseCount, magnitudeRange, magnitudeCount);

    cout << "boundaries OpenMP: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer).count() << " milliseconds" << endl;
#endif

    BoundaryUnion1D boundaryUnion;

    timer = std::chrono::steady_clock::now();

    {
        //The union reads the boundaries through a BoundaryData; building one
        //here used to mean a heap allocation and a matching delete, and the
        //view had to be told it did NOT own what it pointed at.
        const BoundaryData view = boundaryData();
        boundaryUnion.run(&view, m_traceMetadata);
    }

    cout << "1D union: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timer).count() << " milliseconds" << endl;

    m_unionVectors = boundaryUnion.takeUnionVectors();
    m_unionBuckets = boundaryUnion.takeUnionBuckets();

    m_openFlags = boundaryUnion.takeOpenFlags();
    m_upperFlags = boundaryUnion.takeUpperFlags();

    //The trace metadata has been consumed by the 1D union.
    m_traceMetadata.clear();
}

BoundaryData BoundaryEngine::boundaryData(){
    return BoundaryData(m_boundaries, m_openFlags, m_upperFlags, m_phaseCount, m_phaseRange,
                        m_unionVectors, m_unionBuckets, m_magnitudeCount, m_magnitudeRange);
}

std::vector <double> * BoundaryEngine::omega(){
    return m_omega;
}


void BoundaryEngine::traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                                    const BoundarySheets & sheets,
                                    std::map<std::string, TraceLabels> & traceMetadata,
                                    complex <double> p0, const ComplexCloud & valueSet, std::size_t index,
                                    double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom){


    //The map keys are persisted in the .qft files; the loader maps the
    //historical Spanish names of legacy files to these.
    if (m_trackingMask.at(index)){

        TraceLabels & metadata = traceMetadata["Tracking"];

        bound["Tracking"] =
                      traceBoundary(m_specifications.trackingSpreadDb(omega), sheets.at(1),
                                    metadata, p0, valueSet, 1, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_stabilityMask.at(index)){

        TraceLabels & metadata = traceMetadata["Stability"];

        bound["Stability"] =
                      traceBoundary(m_specifications.at(SpecificationType::Stability).boundDb(omega), sheets.at(0),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_noiseMask.at(index)){

        TraceLabels & metadata = traceMetadata["SensorNoise"];

        bound["SensorNoise"] =
                      traceBoundary(m_specifications.at(SpecificationType::SensorNoise).boundDb(omega), sheets.at(0),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_outputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["OutputDisturbance"];

        bound["OutputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega), sheets.at(2),
                                    metadata, p0, valueSet, 2, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_inputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["InputDisturbance"];

        bound["InputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega), sheets.at(3),
                                    metadata, p0, valueSet, 3, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_controlEffortMask.at(index)){

        TraceLabels & metadata = traceMetadata["ControlEffort"];

        bound["ControlEffort"] =
                      traceBoundary(m_specifications.at(SpecificationType::ControlEffort).boundDb(omega), sheets.at(4),
                                    metadata, p0, valueSet, 4, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }
}

#ifdef CUDA_AVAILABLE
void BoundaryEngine::traceFrequency(double omega, std::map<std::string, TraceSet> & bound,
                                    const BoundarySheetsCuda & cudaSheets,
                                    std::map<std::string, TraceLabels> & traceMetadata,
                                    complex <double> p0, const ComplexCloud & valueSet, std::size_t index,
                                    double phaseSpan, double magnitudeSpan, double phaseBottom, double magnitudeBottom){

    if (m_trackingMask.at(index)){

        TraceLabels & metadata = traceMetadata["Tracking"];

        bound["Tracking"] =
                      traceBoundary(m_specifications.trackingSpreadDb(omega), cudaSheets.tracking.data(),
                                    metadata, p0, valueSet, 1, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_stabilityMask.at(index)){

        TraceLabels & metadata = traceMetadata["Stability"];

        bound["Stability"] =
                      traceBoundary(m_specifications.at(SpecificationType::Stability).boundDb(omega), cudaSheets.stabilityNoise.data(),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_noiseMask.at(index)){

        TraceLabels & metadata = traceMetadata["SensorNoise"];

        bound["SensorNoise"] =
                      traceBoundary(m_specifications.at(SpecificationType::SensorNoise).boundDb(omega), cudaSheets.stabilityNoise.data(),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_outputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["OutputDisturbance"];

        bound["OutputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega), cudaSheets.outputDisturbance.data(),
                                    metadata, p0, valueSet, 2, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_inputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["InputDisturbance"];

        bound["InputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega), cudaSheets.inputDisturbance.data(),
                                    metadata, p0, valueSet, 3, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_controlEffortMask.at(index)){

        TraceLabels & metadata = traceMetadata["ControlEffort"];

        bound["ControlEffort"] =
                      traceBoundary(m_specifications.at(SpecificationType::ControlEffort).boundDb(omega), cudaSheets.controlEffort.data(),
                                    metadata, p0, valueSet, 4, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

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


    //Pre-sized and written at index j: the critical section this replaces
    //permuted the metadata against its traces with the thread order.
    traceMetadata.resize(traces.size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t j = 0; j < traces.size(); ++j) {
        //The allowed-side label of the trace: the threshold is the same dB
        //cut the contour was traced at.
        traceMetadata[j] =
                allowedZone(traces.at(j), p0, valueSet, kind, thresholdDb) != 0;
    }

    if (traceMetadata.empty()) {
        traceMetadata.push_back(false);
    }

    return traces;
}


#ifdef CUDA_AVAILABLE
//The CUDA-sheet twin of the function above. It had been left behind by the
//value-semantics work of phase 9: it returned std::vector<std::vector<Point>*>*
//while its own declaration said TraceSet, and it called a tracer overload
//that no longer exists. Neither configuration built it - the sources are
//behind #ifdef CUDA_AVAILABLE and USE_CUDA is off here - so nothing said
//so. Brought in line with the current API; it is the one piece of this
//phase that no compiler on this machine has checked.
TraceSet BoundaryEngine::traceBoundary(double thresholdDb, const float *sheet,
                                       TraceLabels & traceMetadata, std::complex<double> p0,
                                       const ComplexCloud & valueSet, std::int32_t kind,
                                       double phaseSpan, double magnitudeSpan,
                                       double phaseBottom, double magnitudeBottom){

    ContourTracer tracer (thresholdDb, sheet);

    TraceSet traces = tracer.trace(phaseSpan, m_phaseCount, magnitudeSpan,
                                   m_magnitudeCount, phaseBottom, magnitudeBottom);

    traceMetadata.resize(traces.size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t j = 0; j < traces.size(); ++j) {
        traceMetadata[j] =
                allowedZone(traces.at(j), p0, valueSet, kind, thresholdDb) != 0;
    }

    return traces;
}
#endif

std::int32_t BoundaryEngine::allowedZone(const Trace & trace, complex <double> p0, const ComplexCloud & valueSet,
                                   std::int32_t kind, double thresholdDb){

    //Probe point: 1 dB below the trace's maximum magnitude.
    double probeMagnitude = -numeric_limits<double>::infinity();
    double probePhase = -numeric_limits<double>::infinity();

    for (const qftbx::NicholsPoint & point : trace) {
        if(point.magnitude > probeMagnitude){
            probeMagnitude = point.magnitude;
            probePhase = point.phase;
        }
    }

    probeMagnitude -= 1;

    //Nichols (dB, degrees) back to a complex L.
    double linearMagnitude = pow(10, probeMagnitude/20);
    complex<double> L = complex<double> (linearMagnitude * cos (probePhase * M_PI / 180),
                                       linearMagnitude * sin (probePhase * M_PI / 180));


    complex <double> p;
    double dStabilityNoiseCandidate;
    double dOutputDisturbanceCandidate;
    double dInputDisturbanceCandidate;
    double dControlEffortCandidate;

    double dStabilityNoise = -numeric_limits<double>::infinity();
    double dTrackingMin = numeric_limits<double>::infinity();
    double dOutputDisturbance = -numeric_limits<double>::infinity();
    double dInputDisturbance = -numeric_limits<double>::infinity();
    double dControlEffort = -numeric_limits<double>::infinity();


    for (std::size_t h = 0; h < valueSet.size(); ++h) {

        p = valueSet.at(h);

        complex<double> denominator = (p0 / p) + L;

        //Stability and sensor noise share the same transfer magnitude.
        dStabilityNoiseCandidate = abs (L / denominator);

        //Disturbance rejection at the plant output.
        dOutputDisturbanceCandidate = abs ((p0 / p) / denominator);

        //Disturbance rejection at the plant input.
        dInputDisturbanceCandidate = abs (p0 / denominator);

        //Control effort.
        dControlEffortCandidate = abs ((L / p) / denominator);

        if (dStabilityNoiseCandidate > dStabilityNoise){
            dStabilityNoise = dStabilityNoiseCandidate;
        }
        if (dStabilityNoiseCandidate < dTrackingMin){
            dTrackingMin = dStabilityNoiseCandidate;
        }
        if (dOutputDisturbanceCandidate > dOutputDisturbance){
            dOutputDisturbance = dOutputDisturbanceCandidate;
        }
        if (dInputDisturbanceCandidate > dInputDisturbance){
            dInputDisturbance = dInputDisturbanceCandidate;
        }
        if (dControlEffortCandidate > dControlEffort) {
            dControlEffort = dControlEffortCandidate;
        }
    }

    //The sheet is in dB: the zone probe compares in dB too (linear
    //magnitudes used to be compared against dB heights, and tracking as a
    //linear difference against a dB spread).
    switch (kind){
    case 0:
        if (20 * log10(dStabilityNoise) > thresholdDb){
            return 0;
        }
        break;
    case 1:
        if ((20 * log10(dStabilityNoise) - 20 * log10(dTrackingMin)) > thresholdDb){
            return 0;
        }
        break;
    case 2:
        if(20 * log10(dOutputDisturbance) > thresholdDb){
            return 0;
        }
        break;
    case 3:
        if (20 * log10(dInputDisturbance) > thresholdDb){
            return 0;
        }
        break;
    case 4:
        if (20 * log10(dControlEffort) > thresholdDb){
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
    //Base grid of the algorithm.
    const std::vector <double> phases = tools::linspace(phaseRange.min, phaseRange.max, phaseCount);
    const std::vector <double> magnitudes = tools::linspace(magnitudeRange.min, magnitudeRange.max,
                                                      magnitudeCount);

    //Pre-sized containers: every frequency writes at ITS index. The old
    //OpenMP branch created the containers EMPTY and wrote with
    //replace(thread_number) where thread_number was passed BY VALUE (always
    //0): out-of-range writes and zero-sized boundaries in the default
    //build. The caller's frequency vector is no longer touched.
    m_boundaries.assign(static_cast<std::size_t>(omega->size()), {});
    m_traceMetadata.assign(static_cast<std::size_t>(omega->size()), {});

    //Before the threads exist: the loop evaluates the plant, and every
    //evaluation constructs a muParserX parser whose package singletons are
    //built lazily and unsynchronised. See warmUpExpressionParser().
    qftbx::math::warmUpExpressionParser();

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (std::size_t i = 0; i < omega->size(); ++i){

        computeFrequency(omega->at(i), plant, templates.at(i), phases, magnitudes, i);
    }

    m_omega = omega;

}


namespace {

//At the critical grid point L = -1 (phase -180 deg, magnitude 0 dB) the
//nominal plant makes the closed-loop denominator 1 + L vanish: the loop
//has a pole on the imaginary axis and its transfer magnitude is infinite,
//so the cell violates EVERY finite specification. In practice the exact
//zero never occurs because sin(-pi) is -1.2e-16 rather than 0, which
//turns the cell into a finite ~318 dB, and the tracking spread would only
//go NaN (inf - inf) for a single-point value set exactly there. Relying on
//that floating-point accident is not a contract: a non-finite cell is
//stated to violate, since a NaN compares FALSE against the threshold and
//would silently read as ALLOWED. The contour tracer only compares cells
//against the threshold (it never interpolates between them), so an
//infinite cell is safe for it.
double violatingDb(double valueDb)
{
    if (std::isnan(valueDb)) {
        return std::numeric_limits<double>::infinity();
    }

    return valueDb;
}

} // namespace

void BoundaryEngine::computeFrequency (double omega, LtiSystem * plant,
                                       const ComplexCloud & valueSet,
                                       const std::vector <double> & phases,
                                       const std::vector <double> & magnitudes, std::size_t index){

    //Nominal plant at this design frequency.
    complex <double> p0 = plant->evaluate(omega);

    const ComplexCloud & p = valueSet;

    //The five sheets of this frequency, in the order the tracing indexes
    //them, with names for the ones this loop fills.
    BoundarySheets sheets;

    BoundarySheet & stabilityNoiseSheet = sheets.at(0);
    BoundarySheet & trackingSheet = sheets.at(1);
    BoundarySheet & outputDisturbanceSheet = sheets.at(2);
    BoundarySheet & inputDisturbanceSheet = sheets.at(3);
    BoundarySheet & controlEffortSheet = sheets.at(4);

    //One row per MAGNITUDE: the reserve used to ask for the phase count,
    //which is the row WIDTH.
    const std::size_t rowCount = static_cast<std::size_t>(magnitudes.size());
    for (BoundarySheet * sheet : {&stabilityNoiseSheet, &trackingSheet, &outputDisturbanceSheet,
                                 &inputDisturbanceSheet, &controlEffortSheet}) {
        sheet->reserve(rowCount);
    }

    //Second loop variables:
    double magnitudeDb;
    double phaseDegrees;
    double linearMagnitude;
    complex <double> L;
    double dStabilityNoise;
    double dOutputDisturbance;
    double dInputDisturbance;
    double dControlEffort;
    double dTrackingMin;

    //Third loop variables:
    complex <double> pCurrent;
    double dStabilityNoiseCandidate;
    double dOutputDisturbanceCandidate;
    double dInputDisturbanceCandidate;
    double dControlEffortCandidate;
    complex<double> denominator;

    //Grid sweep (no nested parallelism: the outer per-frequency loop is
    //already parallel, and these loops share function-scope variables).
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

            magnitudeDb = magnitudes.at(k);
            phaseDegrees = phases.at(j);

            //Nichols (dB, degrees) to the complex grid point L.
            linearMagnitude = pow(10, magnitudeDb/20);
            L = complex<double> (linearMagnitude * cos (phaseDegrees * M_PI / 180),
                                linearMagnitude * sin (phaseDegrees * M_PI / 180));


            //Template sweep: worst case over the value set at this L.

            dStabilityNoise = -numeric_limits<double>::infinity();
            dTrackingMin = numeric_limits<double>::infinity();
            dOutputDisturbance = -numeric_limits<double>::infinity();
            dInputDisturbance = -numeric_limits<double>::infinity();
            dControlEffort = -numeric_limits<double>::infinity();

            for (std::size_t h = 0; h < p.size(); h++) {

                pCurrent = p[h];

                denominator = (p0 / pCurrent) + L;

                //Stability and sensor noise share the same transfer magnitude.
                dStabilityNoiseCandidate = abs((L / denominator));

                //Disturbance rejection at the plant output.
                dOutputDisturbanceCandidate =  abs((p0 / pCurrent) / denominator);

                //Disturbance rejection at the plant input.
                dInputDisturbanceCandidate = abs((p0 / denominator));

                //Control effort.
                dControlEffortCandidate = abs((L / pCurrent) / denominator);

                if (dStabilityNoiseCandidate > dStabilityNoise){
                    dStabilityNoise = dStabilityNoiseCandidate;
                }
                if (dStabilityNoiseCandidate < dTrackingMin){
                    dTrackingMin = dStabilityNoiseCandidate;
                }
                if (dOutputDisturbanceCandidate > dOutputDisturbance){
                    dOutputDisturbance = dOutputDisturbanceCandidate;
                }
                if (dInputDisturbanceCandidate > dInputDisturbance){
                    dInputDisturbance = dInputDisturbanceCandidate;
                }
                if (dControlEffortCandidate > dControlEffort) {
                    dControlEffort = dControlEffortCandidate;
                }
            }

            //The sheet is ALWAYS stored in dB (contract validated against
            //the golden; the old OpenMP branch stored linear magnitudes and
            //tracking as a linear difference), with the critical point made
            //explicit (see violatingDb).
            stabilityNoiseRow.push_back(violatingDb(20 * log10(dStabilityNoise)));
            trackingRow.push_back(violatingDb((20 * log10(dStabilityNoise)) - (20 * log10(dTrackingMin))));
            outputDisturbanceRow.push_back(violatingDb(20 * log10(dOutputDisturbance)));
            inputDisturbanceRow.push_back(violatingDb(20 * log10(dInputDisturbance)));
            controlEffortRow.push_back(violatingDb(20 * log10(dControlEffort)));
        }
        stabilityNoiseSheet.push_back(std::move(stabilityNoiseRow));
        trackingSheet.push_back(std::move(trackingRow));
        outputDisturbanceSheet.push_back(std::move(outputDisturbanceRow));
        inputDisturbanceSheet.push_back(std::move(inputDisturbanceRow));
        controlEffortSheet.push_back(std::move(controlEffortRow));
    }

    std::map<std::string, TraceSet> bound;

    std::map<std::string, TraceLabels> traceMetadata;

    traceFrequency(omega, bound, sheets, traceMetadata, p0, p, index,
                   m_phaseRange.width(), m_magnitudeRange.width(),
                   m_phaseRange.min, m_magnitudeRange.min);

    //The sheets (~1.7 MB per frequency) die here, which is where they stop
    //being needed: the contours and the zones are extracted. They used to be
    //abandoned with a clear(), then freed by a nested loop over three levels
    //of pointers.

    //Every frequency writes at its own index: no criticals, no permutations.
    m_traceMetadata[index] = std::move(traceMetadata);
    m_boundaries[index] = std::move(bound);
}

} // namespace qftbx
