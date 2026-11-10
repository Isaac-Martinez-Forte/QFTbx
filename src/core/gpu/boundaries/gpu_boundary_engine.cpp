/**
 * @file
 * @brief The boundary engine on the GPU, and the problem of the card.
 *
 * The cell's loop value is taken as nicholsToComplex() takes it: the
 * magnitude's 10^(dB/20) and the cosine and sine of the phase in radians,
 * computed here with the processor's functions, so that the card only
 * multiplies. The record says what the sheets cost on the card: the
 * kernels, the copies back, and in how many batches.
 */

#include "src/core/gpu/boundaries/gpu_boundary_engine.h"

#include <chrono>
#include <cmath>
#include <utility>

#include "src/core/common/exception.h"
#include "src/core/common/record.h"
#include "src/core/gpu/common/cuda_error.h"
#include "src/core/math/constants.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/pipeline/gpu_provider.h"

namespace qftbx::gpu {

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

void GpuBoundaryEngine::compute(std::vector<double> * omega, LtiSystem * plant, const CloudSet & templates,
                                bool templatesAreContours, const SpecificationRecords * specifications,
                                Range phaseRange, std::int32_t phaseCount,
                                Range magnitudeRange, std::int32_t magnitudeCount, double exportInfinity)
{
    (void) exportInfinity;
    const auto started = std::chrono::steady_clock::now();

    BoundaryPlan plan = planBoundaries(*omega, *specifications, phaseRange, phaseCount, magnitudeRange, magnitudeCount);
    const SheetProblem problem = sheetProblemOf(*omega, *plant, templates, templatesAreContours, m_guard, plan,
                                                phaseRange, phaseCount, magnitudeRange, magnitudeCount);

    SheetsOnCard computed;
    try {
        computed = sheetsOnCard(problem, m_cancellation);
    } catch (const CudaError & failure) {
        throw ComputationError(gpuFailure(failure.what()));
    }
    if (computed.cancelled) {
        throw Cancelled();
    }

    record::write("boundaries", "sheets (GPU)",
                  record::milliseconds(std::chrono::duration<double, std::milli>(
                                           std::chrono::steady_clock::now() - started).count())
                      + " " + record::number("frequencies", omega->size())
                      + " " + record::number("kernel-ms", computed.kernelMilliseconds)
                      + " " + record::number("copy-ms", computed.copyMilliseconds)
                      + " " + record::number("batches", computed.batches));

    m_processor.adoptSheets(omega, plant, templates, templatesAreContours, std::move(plan), phaseRange, phaseCount,
                            magnitudeRange, magnitudeCount, std::move(computed.sheets));
}

BoundaryData GpuBoundaryEngine::boundaryData()
{
    return m_processor.boundaryData();
}

}
