#include "boundary_engine.h"

#include "src/core/math/parser_warmup.h"

#include <QElapsedTimer>
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

void BoundaryEngine::compute(QVector<qreal> *omega, LtiSystem *plant, const CloudSet & templates,
                             const qftbx::SpecificationRecords * specifications, QPointF phaseRange, qint32 phaseCount, QPointF magnitudeRange,
                             qint32 magnitudeCount, qreal exportInfinity, bool cuda){

    //The export stand-in for infinity is not part of the computation
    //(thesis ch. 7: it exists so exported data can carry a finite value in
    //formats that cannot represent an infinity). It is kept in the
    //interface for the numeric export, still to be implemented.
    Q_UNUSED(exportInfinity);



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
    foreach(qreal o, *omega){
        m_trackingMask.append(m_specifications.at(SpecificationType::TrackingLower).appliesAt(o));
        m_stabilityMask.append(m_specifications.at(SpecificationType::Stability).appliesAt(o));
        m_noiseMask.append(m_specifications.at(SpecificationType::SensorNoise).appliesAt(o));
        m_outputDisturbanceMask.append(m_specifications.at(SpecificationType::OutputDisturbance).appliesAt(o));
        m_inputDisturbanceMask.append(m_specifications.at(SpecificationType::InputDisturbance).appliesAt(o));
        m_controlEffortMask.append(m_specifications.at(SpecificationType::ControlEffort).appliesAt(o));
    }

    if (m_trackingMask.contains(true) &&
            !m_specifications.at(SpecificationType::TrackingUpper).used()){
        //The historical code dereferenced T_U's null plant.
        throw InvalidInput("The tracking boundary needs both tracking "
                           "specifications (T_L and T_U).");
    }



#ifdef CUDA_AVAILABLE

    QElapsedTimer timer;
    timer.start();

    if (!cuda){

        computeFrequencies(omega, plant, templates, phaseRange,
                           phaseCount, magnitudeRange, magnitudeCount);

        cout << "boundaries OpenMP: " << timer.elapsed() << " milliseconds" << endl;

    } else {


        m_boundaries.clear();
        m_traceMetadata.clear();

        for (int i = 0; i < omega->size(); i++){

            std::complex <qreal> p0 = plant->evaluate(omega->at(i));
            const ComplexCloud & valueSet = templates.at(static_cast<std::size_t>(i));

            //Sheets come back by value in one struct: the old raw float*
            //vector leaked all five sheets on every frequency.
            const BoundarySheetsCuda cudaSheets = boundarySheetsCuda(
                valueSet, p0,
                tools::linspace1(phaseRange.x(), phaseRange.y(), phaseCount),
                tools::linspace1(magnitudeRange.x(), magnitudeRange.y(), magnitudeCount));

            std::map<QString, TraceSet> bound;

            std::map<QString, TraceLabels> traceMetadata;

            traceFrequency(omega->at(i), bound, cudaSheets, traceMetadata, p0, valueSet, i,
                           phaseRange.y() - phaseRange.x(), magnitudeRange.y() - magnitudeRange.x(),
                           phaseRange.x(), magnitudeRange.x());

            m_traceMetadata.push_back(std::move(traceMetadata));
            m_boundaries.push_back(std::move(bound));
        }

        //Alias of the caller's vector, like the CPU path (the old copy was
        //the one leak releaseResults could not free).
        m_omega = omega;

        cout << "boundaries CUDA: " << timer.elapsed() << " milliseconds" << endl;


    }
#else
    QElapsedTimer timer;
    timer.start();

    computeFrequencies(omega, plant, templates, phaseRange, phaseCount, magnitudeRange, magnitudeCount);

    cout << "boundaries OpenMP: " << timer.elapsed() << " milliseconds" << endl;
#endif

    BoundaryUnion1D boundaryUnion;

    timer.restart();

    {
        //The union reads the boundaries through a BoundaryData; building one
        //here used to mean a heap allocation and a matching delete, and the
        //view had to be told it did NOT own what it pointed at.
        const BoundaryData view = boundaryData();
        boundaryUnion.run(&view, m_traceMetadata);
    }

    cout << "1D union: " << timer.elapsed() << " milliseconds" << endl;

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

QVector <qreal> * BoundaryEngine::omega(){
    return m_omega;
}


void BoundaryEngine::traceFrequency(qreal omega, std::map<QString, TraceSet> & bound,
                                    QVector <QVector <QVector <qreal> * > * > * sheets,
                                    std::map<QString, TraceLabels> & traceMetadata,
                                    complex <qreal> p0, const ComplexCloud & valueSet, qint32 index,
                                    qreal phaseSpan, qreal magnitudeSpan, qreal phaseBottom, qreal magnitudeBottom){


    //The map keys are persisted in the .qft files; the loader maps the
    //historical Spanish names of legacy files to these.
    if (m_trackingMask.at(index)){

        TraceLabels & metadata = traceMetadata["Tracking"];

        bound["Tracking"] =
                      traceBoundary(m_specifications.trackingSpreadDb(omega), sheets->at(1),
                                    metadata, p0, valueSet, 1, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_stabilityMask.at(index)){

        TraceLabels & metadata = traceMetadata["Stability"];

        bound["Stability"] =
                      traceBoundary(m_specifications.at(SpecificationType::Stability).boundDb(omega), sheets->at(0),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_noiseMask.at(index)){

        TraceLabels & metadata = traceMetadata["SensorNoise"];

        bound["SensorNoise"] =
                      traceBoundary(m_specifications.at(SpecificationType::SensorNoise).boundDb(omega), sheets->at(0),
                                    metadata, p0, valueSet, 0, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_outputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["OutputDisturbance"];

        bound["OutputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::OutputDisturbance).boundDb(omega), sheets->at(2),
                                    metadata, p0, valueSet, 2, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_inputDisturbanceMask.at(index)){

        TraceLabels & metadata = traceMetadata["InputDisturbance"];

        bound["InputDisturbance"] =
                      traceBoundary(m_specifications.at(SpecificationType::InputDisturbance).boundDb(omega), sheets->at(3),
                                    metadata, p0, valueSet, 3, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }

    if (m_controlEffortMask.at(index)){

        TraceLabels & metadata = traceMetadata["ControlEffort"];

        bound["ControlEffort"] =
                      traceBoundary(m_specifications.at(SpecificationType::ControlEffort).boundDb(omega), sheets->at(4),
                                    metadata, p0, valueSet, 4, phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);
    }
}

#ifdef CUDA_AVAILABLE
void BoundaryEngine::traceFrequency(qreal omega, std::map<QString, TraceSet> & bound,
                                    const BoundarySheetsCuda & cudaSheets,
                                    std::map<QString, TraceLabels> & traceMetadata,
                                    complex <qreal> p0, const ComplexCloud & valueSet, qint32 index,
                                    qreal phaseSpan, qreal magnitudeSpan, qreal phaseBottom, qreal magnitudeBottom){

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

TraceSet BoundaryEngine::traceBoundary(qreal thresholdDb, QVector<QVector<qreal> *> *sheet,
                                       TraceLabels & traceMetadata, std::complex<qreal> p0,
                                       const ComplexCloud & valueSet,
                                       qint32 kind, qreal phaseSpan, qreal magnitudeSpan,
                                       qreal phaseBottom, qreal magnitudeBottom)
{

    ContourTracer tracer (thresholdDb, sheet);

    TraceSet traces = tracer.trace(phaseSpan, magnitudeSpan, phaseBottom, magnitudeBottom);


    //Pre-sized and written at index j: the critical section this replaces
    //permuted the metadata against its traces with the thread order.
    traceMetadata.resize(traces.size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 j = 0; j < static_cast<qint32>(traces.size()); j++) {
        QPoint label;
        //The allowed-side label of the trace: the threshold is the same dB
        //cut the contour was traced at.
        label.setX(allowedZone(traces.at(static_cast<std::size_t>(j)), p0, valueSet, kind, thresholdDb));

        traceMetadata[static_cast<std::size_t>(j)] = label;
    }

    if (traceMetadata.empty()) {
        traceMetadata.push_back(QPoint(0,0));
    }

    return traces;
}


#ifdef CUDA_AVAILABLE
QVector<QVector<QPointF> *> * BoundaryEngine::traceBoundary(qreal thresholdDb, const float *sheet,
                                                            QVector<QPoint> *traceMetadata, std::complex<qreal> p0,
                                                            const ComplexCloud & valueSet, qint32 kind,
                                                            qreal phaseSpan, qreal magnitudeSpan, qreal phaseBottom, qreal magnitudeBottom){

    ContourTracer tracer (thresholdDb, sheet);

    QVector<QVector<QPointF> *> * traces = tracer.trace(phaseSpan, m_phaseCount, magnitudeSpan, m_magnitudeCount, phaseBottom, magnitudeBottom);


    traceMetadata->resize(traces->size());

#ifdef OpenMP_AVAILABLE
#pragma omp parallel for
#endif
    for (qint32 j = 0; j < traces->size(); j++) {
        QPoint label;
        label.setX(allowedZone(traces->at(j), p0, valueSet, kind, thresholdDb));

        traceMetadata->replace(j, label);
    }

    return traces;
}
#endif

qint32 BoundaryEngine::allowedZone(const Trace & trace, complex <qreal> p0, const ComplexCloud & valueSet,
                                   qint32 kind, qreal thresholdDb){

    //Probe point: 1 dB below the trace's maximum magnitude.
    qreal probeMagnitude = -numeric_limits<qreal>::infinity();
    qreal probePhase = -numeric_limits<qreal>::infinity();

    for (const QPointF & point : trace) {
        if(point.y() > probeMagnitude){
            probeMagnitude = point.y();
            probePhase = point.x();
        }
    }

    probeMagnitude -= 1;

    //Nichols (dB, degrees) back to a complex L.
    qreal linearMagnitude = pow(10, probeMagnitude/20);
    complex<qreal> L = complex<qreal> (linearMagnitude * cos (probePhase * M_PI / 180),
                                       linearMagnitude * sin (probePhase * M_PI / 180));


    complex <qreal> p;
    qreal dStabilityNoiseCandidate;
    qreal dOutputDisturbanceCandidate;
    qreal dInputDisturbanceCandidate;
    qreal dControlEffortCandidate;

    qreal dStabilityNoise = -numeric_limits<qreal>::infinity();
    qreal dTrackingMin = numeric_limits<qreal>::infinity();
    qreal dOutputDisturbance = -numeric_limits<qreal>::infinity();
    qreal dInputDisturbance = -numeric_limits<qreal>::infinity();
    qreal dControlEffort = -numeric_limits<qreal>::infinity();


    for (qint32 h = 0; h < valueSet.size(); h++) {

        p = valueSet.at(h);

        complex<qreal> denominator = (p0 / p) + L;

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

void BoundaryEngine::computeFrequencies(QVector<qreal> *omega, LtiSystem *plant,
                                        const CloudSet & templates, QPointF phaseRange, qint32 phaseCount,
                                        QPointF magnitudeRange, qint32 magnitudeCount)
{
    //Base grid of the algorithm.
    const QVector <qreal> phases = tools::linspace(phaseRange.x(), phaseRange.y(), phaseCount);
    const QVector <qreal> magnitudes = tools::linspace(magnitudeRange.x(), magnitudeRange.y(),
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
    for (qint32 i = 0; i < omega->size(); i++){

        computeFrequency(omega->at(i), plant, templates.at(static_cast<std::size_t>(i)), phases, magnitudes, i);
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
qreal violatingDb(qreal valueDb)
{
    if (std::isnan(valueDb)) {
        return std::numeric_limits<qreal>::infinity();
    }

    return valueDb;
}

} // namespace

void BoundaryEngine::computeFrequency (qreal omega, LtiSystem * plant,
                                       const ComplexCloud & valueSet,
                                       const QVector <qreal> & phases,
                                       const QVector <qreal> & magnitudes, qint32 index){

    //Nominal plant at this design frequency.
    complex <qreal> p0 = plant->evaluate(omega);

    const ComplexCloud & p = valueSet;

    //One sheet per specification family and frequency.
    QVector <QVector <qreal> * > * stabilityNoiseSheet = new QVector <QVector <qreal> * > ();
    stabilityNoiseSheet->reserve(phases.size());

    QVector <QVector <qreal> * > * trackingSheet = new QVector <QVector <qreal> * > ();
    trackingSheet->reserve(phases.size());

    QVector <QVector <qreal> * > * outputDisturbanceSheet = new QVector <QVector <qreal> * > ();
    outputDisturbanceSheet->reserve(phases.size());

    QVector <QVector <qreal> * > * inputDisturbanceSheet = new QVector <QVector <qreal> * > ();
    inputDisturbanceSheet->reserve(phases.size());

    QVector <QVector <qreal> * > * controlEffortSheet = new QVector <QVector <qreal> * > ();
    controlEffortSheet->reserve(phases.size());

    //First loop variables:
    QVector <qreal> * stabilityNoiseRow;
    QVector <qreal> * trackingRow;
    QVector <qreal> * outputDisturbanceRow;
    QVector <qreal> * inputDisturbanceRow;
    QVector <qreal> * controlEffortRow;

    //Second loop variables:
    qreal magnitudeDb;
    qreal phaseDegrees;
    qreal linearMagnitude;
    complex <qreal> L;
    qreal dStabilityNoise;
    qreal dOutputDisturbance;
    qreal dInputDisturbance;
    qreal dControlEffort;
    qreal dTrackingMin;

    //Third loop variables:
    complex <qreal> pCurrent;
    qreal dStabilityNoiseCandidate;
    qreal dOutputDisturbanceCandidate;
    qreal dInputDisturbanceCandidate;
    qreal dControlEffortCandidate;
    complex<qreal> denominator;

    //Grid sweep (no nested parallelism: the outer per-frequency loop is
    //already parallel, and these loops share function-scope variables).
    for (qint32 k = 0; k < magnitudes.size(); k++){

        stabilityNoiseRow = new QVector <qreal> ();
        stabilityNoiseRow->reserve(phases.size());

        trackingRow = new QVector <qreal> ();
        trackingRow->reserve(phases.size());

        outputDisturbanceRow = new QVector <qreal> ();
        outputDisturbanceRow->reserve(phases.size());

        inputDisturbanceRow = new QVector <qreal> ();
        inputDisturbanceRow->reserve(phases.size());

        controlEffortRow = new QVector <qreal> ();
        controlEffortRow->reserve(phases.size());

        for (qint32 j = 0; j < phases.size(); j++){

            magnitudeDb = magnitudes.at(k);
            phaseDegrees = phases.at(j);

            //Nichols (dB, degrees) to the complex grid point L.
            linearMagnitude = pow(10, magnitudeDb/20);
            L = complex<qreal> (linearMagnitude * cos (phaseDegrees * M_PI / 180),
                                linearMagnitude * sin (phaseDegrees * M_PI / 180));


            //Template sweep: worst case over the value set at this L.

            dStabilityNoise = -numeric_limits<qreal>::infinity();
            dTrackingMin = numeric_limits<qreal>::infinity();
            dOutputDisturbance = -numeric_limits<qreal>::infinity();
            dInputDisturbance = -numeric_limits<qreal>::infinity();
            dControlEffort = -numeric_limits<qreal>::infinity();

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
            stabilityNoiseRow->append(violatingDb(20 * log10(dStabilityNoise)));
            trackingRow->append(violatingDb((20 * log10(dStabilityNoise)) - (20 * log10(dTrackingMin))));
            outputDisturbanceRow->append(violatingDb(20 * log10(dOutputDisturbance)));
            inputDisturbanceRow->append(violatingDb(20 * log10(dInputDisturbance)));
            controlEffortRow->append(violatingDb(20 * log10(dControlEffort)));
        }
        stabilityNoiseSheet->append(stabilityNoiseRow);
        trackingSheet->append(trackingRow);
        outputDisturbanceSheet->append(outputDisturbanceRow);
        inputDisturbanceSheet->append(inputDisturbanceRow);
        controlEffortSheet->append(controlEffortRow);
    }

    std::map<QString, TraceSet> bound;

    std::map<QString, TraceLabels> traceMetadata;

    //All sheets packed together, indexed as traceFrequency expects them.
    QVector <QVector <QVector <qreal> * > * > * sheets = new QVector <QVector <QVector <qreal> * > * > ();

    sheets->append(stabilityNoiseSheet);
    sheets->append(trackingSheet);
    sheets->append(outputDisturbanceSheet);
    sheets->append(inputDisturbanceSheet);
    sheets->append(controlEffortSheet);


    traceFrequency(omega, bound, sheets, traceMetadata, p0, p, index,
                   m_phaseRange.y() - m_phaseRange.x(), m_magnitudeRange.y() - m_magnitudeRange.x(),
                   m_phaseRange.x(), m_magnitudeRange.x());

    //The sheets (~1.7 MB per frequency) are no longer needed: the contours
    //and zones are extracted. They used to be abandoned with a clear().
    foreach (QVector <QVector <qreal> * > * sheet, *sheets){
        foreach (QVector <qreal> * row, *sheet){
            delete row;
        }
        delete sheet;
    }
    delete sheets;

    //Every frequency writes at its own index: no criticals, no permutations.
    m_traceMetadata[static_cast<std::size_t>(index)] = std::move(traceMetadata);
    m_boundaries[static_cast<std::size_t>(index)] = std::move(bound);
}

} // namespace qftbx
