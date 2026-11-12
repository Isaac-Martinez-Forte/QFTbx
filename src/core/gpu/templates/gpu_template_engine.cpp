/**
 * @file
 * @brief The template engine on the GPU, and the description of a plant
 * for the card.
 *
 * Everything but the check of the family's poles and the sweep goes to the
 * engine of the processor, which keeps the settings, the clouds and what it
 * computes from them. The record says what each cost on the card: for the
 * check, its denominators and its kernels; for the sweep, the kernels, the
 * copies back and in how many batches.
 */

#include "src/core/gpu/templates/gpu_template_engine.h"

#include <chrono>
#include <string>

#include "src/core/common/exception.h"
#include "src/core/common/record.h"
#include "src/core/gpu/common/cuda_error.h"
#include "src/core/gpu/templates/family_poles.h"
#include "src/core/gpu/templates/plant_sweep.h"
#include "src/core/pipeline/gpu_provider.h"
#include "src/core/system/free_form.h"

namespace qftbx::gpu {

namespace {

std::vector<std::int32_t> slotsOf(const std::vector<std::size_t> & slots)
{
    return std::vector<std::int32_t>(slots.begin(), slots.end());
}

FamilyPoles familyPolesOf(const FamilyOnCard & family)
{
    FamilyPoles poles;
    poles.firstCount = family.firstCount;
    if (family.unplaceableAt.has_value()) {
        poles.unplaceableAt = static_cast<std::size_t>(*family.unplaceableAt);
    }
    if (family.otherCountAt.has_value()) {
        poles.otherCountAt = static_cast<std::size_t>(*family.otherCountAt);
    }
    poles.otherCount = family.otherCount;
    poles.bands = family.bands;
    return poles;
}

}

PlantModel modelOf(LtiSystem & plant, const SweepPlan & plan)
{
    PlantModel model;
    model.numeratorSlot = plan.numeratorSlot;
    model.numeratorNominal = plan.numeratorNominal;
    model.denominatorSlot = plan.denominatorSlot;
    model.denominatorNominal = plan.denominatorNominal;
    model.gainSlot = plan.gainSlot;
    model.delaySlot = plan.delaySlot;
    model.gainNominal = plan.gainNominal;
    model.delayNominal = plan.delayNominal;

    switch (plant.type()) {
    case LtiSystem::SystemType::PolynomialForm:
        model.form = PlantForm::Polynomial;
        break;
    case LtiSystem::SystemType::ZeroPoleGain:
        model.form = PlantForm::ZeroPoleGain;
        break;
    case LtiSystem::SystemType::TimeConstantGain:
        model.form = PlantForm::TimeConstantGain;
        break;
    case LtiSystem::SystemType::FreeForm: {
        model.form = PlantForm::FreeForm;
        const FreeForm::Evaluation evaluation = dynamic_cast<FreeForm &>(plant).evaluation();
        model.program = evaluation.ratio.steps;
        model.programDepth = static_cast<std::int32_t>(evaluation.ratio.depth);
        model.denominatorProgram = evaluation.denominator.steps;
        model.denominatorProgramDepth = static_cast<std::int32_t>(evaluation.denominator.depth);
        model.numeratorValueSlot = slotsOf(evaluation.numeratorSlots);
        model.denominatorValueSlot = slotsOf(evaluation.denominatorSlots);
        model.valueCount = static_cast<std::int32_t>(evaluation.valueCount);
        break;
    }
    }

    if (plan.grids.size() > static_cast<std::size_t>(kMaxDigits)) {
        throw ComputationError(gpuFailure("the plant has " + std::to_string(plan.grids.size())
                                          + " uncertain parameters, and a thread of the card holds "
                                          + std::to_string(kMaxDigits)));
    }
    if (model.valueCount > kMaxValues || model.programDepth > kMaxDepth || model.denominatorProgramDepth > kMaxDepth) {
        throw ComputationError(gpuFailure("the plant's expression needs more values or more stack than a thread of the card holds"));
    }
    if (model.form != PlantForm::FreeForm && model.denominatorSlot.size() > static_cast<std::size_t>(kMaxDenominator)) {
        throw ComputationError(gpuFailure("the plant's denominator has " + std::to_string(model.denominatorSlot.size())
                                          + " coefficients or factors, and a warp of the card holds "
                                          + std::to_string(kMaxDenominator)));
    }
    if (plan.combinationCount >= (std::size_t{1} << kCombinationBits)) {
        throw ComputationError(gpuFailure("the family has more combinations than the card counts"));
    }
    return model;
}

SweepGrid gridOf(const SweepPlan & plan)
{
    SweepGrid grid;
    grid.grids = plan.grids;
    grid.combinations = plan.combinationCount;
    grid.border = plan.border;
    return grid;
}

void GpuTemplateEngine::sweep(LtiSystem & plant, const std::vector<double> & frequencies)
{
    const auto started = std::chrono::steady_clock::now();
    SweepPlan plan = planCombinations(plant, frequencies, m_grids, m_borderSweep);
    const PlantModel model = modelOf(plant, plan);
    const SweepGrid grid = gridOf(plan);

    const auto checking = std::chrono::steady_clock::now();
    FamilyOnCard family;
    try {
        family = familyOnCard(model, grid);
    } catch (const CudaError & failure) {
        throw ComputationError(gpuFailure(failure.what()));
    }
    adoptFamily(plan, familyPolesOf(family), frequencies);
    record::write("templates", "family (GPU)",
                  record::milliseconds(std::chrono::duration<double, std::milli>(
                                           std::chrono::steady_clock::now() - checking).count())
                      + " " + record::number("denominators", family.denominators)
                      + " " + record::number("kernel-ms", family.kernelMilliseconds));

    SweepOnCard swept;
    try {
        swept = sweepOnCard(model, grid, frequencies, m_cancellation);
    } catch (const CudaError & failure) {
        throw ComputationError(gpuFailure(failure.what()));
    }
    if (swept.cancelled) {
        throw Cancelled();
    }
    refuseNonFinite(frequencies, swept.nonFinite);

    std::size_t points = 0;
    for (const ComplexCloud & cloud : swept.clouds) {
        points += cloud.size();
    }
    m_resident = swept.resident;
    m_processor.adoptSweep(std::move(swept.clouds), plan, frequencies);

    record::write("templates", "sweep (GPU)",
                  record::milliseconds(std::chrono::duration<double, std::milli>(
                                           std::chrono::steady_clock::now() - started).count())
                      + " " + record::number("frequencies", frequencies.size())
                      + " " + record::number("points", points)
                      + " " + record::number("kernel-ms", swept.kernelMilliseconds)
                      + " " + record::number("copy-ms", swept.copyMilliseconds)
                      + " " + record::number("batches", swept.batches));
}

void GpuTemplateEngine::setCancellation(const CancellationToken * token)
{
    m_cancellation = token;
    m_processor.setCancellation(token);
}

void GpuTemplateEngine::setHullMetric(HullMetric metric, double dbPerDegree)
{
    m_processor.setHullMetric(metric, dbPerDegree);
}

void GpuTemplateEngine::setWholeCloudStandsIn(bool standsIn)
{
    m_processor.setWholeCloudStandsIn(standsIn);
}

void GpuTemplateEngine::setAlphaShapeContour(bool alphaShape)
{
    m_processor.setAlphaShapeContour(alphaShape);
}

void GpuTemplateEngine::setBorderSweep(bool border)
{
    m_borderSweep = border;
    m_processor.setBorderSweep(border);
}

void GpuTemplateEngine::setGrids(ParameterGrids grids)
{
    m_grids = grids;
    m_processor.setGrids(std::move(grids));
}

void GpuTemplateEngine::setEpsilon(std::vector<double> epsilon)
{
    m_processor.setEpsilon(std::move(epsilon));
}

void GpuTemplateEngine::setClouds(CloudSet clouds)
{
    m_resident.reset();
    m_processor.setClouds(std::move(clouds));
}

bool GpuTemplateEngine::compute(LtiSystem * plant, std::vector<double> * frequencies)
{
    sweep(*plant, *frequencies);
    return m_processor.contourClouds();
}

CloudSet GpuTemplateEngine::computeClouds(LtiSystem * plant, std::vector<double> * frequencies)
{
    sweep(*plant, *frequencies);
    return m_processor.clouds();
}

bool GpuTemplateEngine::computeContours(std::vector<double> epsilon)
{
    return m_processor.computeContours(std::move(epsilon));
}

std::vector<EpsilonProposal> GpuTemplateEngine::proposeEpsilon()
{
    return m_processor.proposeEpsilon();
}

const CloudSet & GpuTemplateEngine::clouds() const
{
    return m_processor.clouds();
}

const CloudSet & GpuTemplateEngine::contours() const
{
    return m_processor.contours();
}

const std::vector<double> & GpuTemplateEngine::epsilon() const
{
    return m_processor.epsilon();
}

const std::vector<ContourReport> & GpuTemplateEngine::contourReports() const
{
    return m_processor.contourReports();
}

const std::optional<PoleCountChange> & GpuTemplateEngine::poleCountChange() const
{
    return m_processor.poleCountChange();
}

}
