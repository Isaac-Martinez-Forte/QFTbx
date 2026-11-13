/**
 * @file
 * @brief The boundary engine on the GPU, and the problem of the card.
 *
 * The cell's loop value is taken as nicholsToComplex() takes it: the
 * magnitude's 10^(dB/20) and the cosine and sine of the phase in radians,
 * computed here with the processor's functions, so that the card only
 * multiplies. The record says what the boundaries cost on the card: the
 * kernels, the copies back, in how many batches, and whether the clouds
 * were read where the card held them.
 */

#include "src/core/gpu/boundaries/gpu_boundary_engine.h"

#include <chrono>
#include <cmath>
#include <utility>

#include "src/core/common/exception.h"
#include "src/core/common/record.h"
#include "src/core/gpu/common/cuda_error.h"
#include "src/core/gpu/templates/walk_preparation.h"
#include "src/core/math/constants.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/pipeline/gpu_provider.h"

namespace qftbx::gpu {

namespace {

bool holds(const std::shared_ptr<const ResidentClouds> & resident, const CloudSet & templates, bool contours)
{
    if (resident == nullptr || contours || resident->frequencies() != templates.size()) {
        return false;
    }
    for (const ComplexCloud & cloud : templates) {
        if (cloud.size() != resident->plants()) {
            return false;
        }
    }
    return true;
}

}

std::vector<ReadingRequest> requestsOf(const std::vector<SheetReading> & named)
{
    std::vector<ReadingRequest> requests;
    for (const SheetReading & reading : named) {
        requests.push_back({reading.sheet, reading.thresholdDb});
    }
    return requests;
}

FrequencyBoundaries boundariesOf(const std::vector<SheetReading> & named, const std::vector<ReadingOnCard> & read,
                                Range phaseRange, std::int32_t phaseCount, Range magnitudeRange,
                                std::int32_t magnitudeCount)
{
    const double phaseSpan = phaseRange.width();
    const double magnitudeSpan = magnitudeRange.width();
    const std::int32_t phaseCells = phaseCount - 1;
    const std::int32_t magnitudeCells = magnitudeCount - 1;
    const auto toNichols = [&](std::int32_t x, std::int32_t y) {
        return NicholsPoint(((x * phaseSpan) / phaseCells) + phaseRange.min,
                            ((y * magnitudeSpan) / magnitudeCells) + magnitudeRange.min);
    };

    FrequencyBoundaries boundaries;
    for (std::size_t k = 0; k < named.size(); ++k) {
        const ReadingOnCard & reading = read.at(k);
        TraceSet traces;
        TraceLabels labels;
        for (std::size_t t = 0; t + 1 < reading.traceStart.size(); ++t) {
            Trace trace;
            for (std::int32_t p = reading.traceStart[t]; p < reading.traceStart[t + 1]; ++p) {
                trace.push_back(toNichols(reading.x[static_cast<std::size_t>(p)], reading.y[static_cast<std::size_t>(p)]));
            }
            trace.insert(trace.begin(), NicholsPoint(trace.front().phase - (phaseSpan / phaseCells), trace.front().magnitude));
            trace.push_back(NicholsPoint(trace.back().phase + (phaseSpan / phaseCells), trace.back().magnitude));
            traces.push_back(std::move(trace));
            labels.push_back(reading.allowedBelow.at(t) != 0);
        }
        if (labels.empty()) {
            labels.push_back(false);
        }

        std::vector<std::vector<BoundaryColumns::Span>> columns(static_cast<std::size_t>(phaseCount));
        for (std::int32_t c = 0; c < phaseCount; ++c) {
            for (std::int32_t i = reading.columnBegin.at(static_cast<std::size_t>(c));
                 i < reading.columnBegin.at(static_cast<std::size_t>(c) + 1); ++i) {
                columns[static_cast<std::size_t>(c)].push_back({reading.lo[static_cast<std::size_t>(i)],
                                                                reading.hi[static_cast<std::size_t>(i)]});
            }
        }

        const std::string & name = named[k].name;
        boundaries.traces[name] = std::move(traces);
        boundaries.labels[name] = std::move(labels);
        boundaries.columns[name] = BoundaryColumns(std::move(columns), phaseCount, phaseRange);
    }
    return boundaries;
}

SheetProblem sheetProblemOf(const std::vector<double> & omega, LtiSystem & plant, const CloudSet & templates,
                            bool templatesAreContours, bool guard, const BoundaryPlan & plan,
                            Range phaseRange, std::int32_t phaseCount, Range magnitudeRange, std::int32_t magnitudeCount)
{
    SheetProblem problem;
    problem.templates = &templates;
    problem.contours = templatesAreContours;
    problem.guard = guard;
    for (std::size_t u = 0; u < omega.size(); ++u) {
        problem.nominal.push_back(plant.evaluate(omega[u]));
        problem.sheets.push_back(plan.sheetsUsed(u));
    }
    for (const double magnitude : linspace(magnitudeRange.min, magnitudeRange.max, magnitudeCount)) {
        problem.rowMagnitudes.push_back(std::pow(10.0, magnitude / 20.0));
    }
    for (const double phase : linspace(phaseRange.min, phaseRange.max, phaseCount)) {
        const double radians = phase * math::kPi / 180.0;
        problem.cosines.push_back(std::cos(radians));
        problem.sines.push_back(std::sin(radians));
    }
    return problem;
}

void GpuBoundaryEngine::setCancellation(const CancellationToken * token)
{
    m_cancellation = token;
    m_processor.setCancellation(token);
}

void GpuBoundaryEngine::setSingularLocusGuard(bool on)
{
    m_guard = on;
    m_processor.setSingularLocusGuard(on);
}

void GpuBoundaryEngine::setClosedFormColumns(bool on)
{
    m_processor.setClosedFormColumns(on);
}

void GpuBoundaryEngine::setResidentClouds(std::shared_ptr<const ResidentClouds> clouds)
{
    m_resident = std::move(clouds);
}

void GpuBoundaryEngine::compute(std::vector<double> * omega, LtiSystem * plant, const CloudSet & templates,
                                bool templatesAreContours, const SpecificationRecords * specifications,
                                Range phaseRange, std::int32_t phaseCount,
                                Range magnitudeRange, std::int32_t magnitudeCount, double exportInfinity)
{
    (void) exportInfinity;
    const auto started = std::chrono::steady_clock::now();

    BoundaryPlan plan = planBoundaries(*omega, *specifications, phaseRange, phaseCount, magnitudeRange, magnitudeCount);
    SheetProblem problem = sheetProblemOf(*omega, *plant, templates, templatesAreContours, m_guard, plan,
                                          phaseRange, phaseCount, magnitudeRange, magnitudeCount);
    const std::shared_ptr<const DeviceClouds> resident = holds(m_resident, templates, templatesAreContours)
            ? deviceCloudsOf(m_resident) : nullptr;
    problem.resident = resident.get();

    std::vector<std::vector<SheetReading>> named;
    std::vector<std::vector<ReadingRequest>> requests;
    for (std::size_t u = 0; u < omega->size(); ++u) {
        named.push_back(plan.readings(u, omega->at(u)));
        requests.push_back(requestsOf(named.back()));
    }

    BoundariesOnCard computed;
    try {
        computed = boundariesOnCard(problem, requests, phaseRange, magnitudeRange, m_cancellation);
    } catch (const CudaError & failure) {
        throw ComputationError(gpuFailure(failure.what()));
    }
    if (computed.cancelled) {
        throw Cancelled();
    }

    std::vector<FrequencyBoundaries> boundaries;
    for (std::size_t u = 0; u < omega->size(); ++u) {
        boundaries.push_back(boundariesOf(named[u], computed.readings[u], phaseRange, phaseCount, magnitudeRange,
                                          magnitudeCount));
    }

    record::write("boundaries", "sheets and readings (GPU)",
                  record::milliseconds(std::chrono::duration<double, std::milli>(
                                           std::chrono::steady_clock::now() - started).count())
                      + " " + record::number("frequencies", omega->size())
                      + " " + record::number("kernel-ms", computed.kernelMilliseconds)
                      + " " + record::number("copy-ms", computed.copyMilliseconds)
                      + " " + record::number("batches", computed.batches)
                      + " " + record::number("resident", problem.resident != nullptr ? 1 : 0));

    m_processor.adoptBoundaries(omega, plant, templates, templatesAreContours, std::move(plan), phaseRange, phaseCount,
                                magnitudeRange, magnitudeCount, std::move(boundaries));
}

BoundaryData GpuBoundaryEngine::boundaryData()
{
    return m_processor.boundaryData();
}

}
