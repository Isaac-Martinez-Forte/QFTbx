/**
 * @file
 * @brief The template engine on the GPU.
 *
 * It plans the sweep as the processor does (planSweep), refusing the same
 * families, sweeps the family on the card and hands the clouds over to an
 * engine of the processor for what has no GPU code yet, the proposal of
 * epsilon and the contours. The clouds stay on the card when they fit, for
 * the phases after. A failure on the card is a ComputationError with
 * CUDA's text. modelOf() describes the plant for the card and refuses one
 * whose expression or parameters do not fit a thread; gridOf() is the
 * plan's combinations.
 */

#ifndef QFTBX_GPU_TEMPLATE_ENGINE_H
#define QFTBX_GPU_TEMPLATE_ENGINE_H

#include <memory>
#include <vector>

#include "src/core/gpu/templates/plant_model.h"
#include "src/core/templates/sweep_plan.h"
#include "src/core/templates/template_engine.h"

namespace qftbx::gpu {

class DeviceClouds;

class GpuTemplateEngine : public TemplateComputation
{
public:
    void setCancellation(const CancellationToken * token) override;
    void setHullMetric(HullMetric metric, double dbPerDegree) override;
    void setWholeCloudStandsIn(bool standsIn) override;
    void setAlphaShapeContour(bool alphaShape) override;
    void setBorderSweep(bool border) override;
    void setGrids(ParameterGrids grids) override;
    void setEpsilon(std::vector<double> epsilon) override;
    void setClouds(CloudSet clouds) override;

    bool compute(LtiSystem * plant, std::vector<double> * frequencies) override;
    CloudSet computeClouds(LtiSystem * plant, std::vector<double> * frequencies) override;
    bool computeContours(std::vector<double> epsilon) override;
    std::vector<EpsilonProposal> proposeEpsilon() override;

    const CloudSet & clouds() const override;
    const CloudSet & contours() const override;
    const std::vector<double> & epsilon() const override;
    const std::vector<ContourReport> & contourReports() const override;

    const std::shared_ptr<const DeviceClouds> & resident() const { return m_resident; }

private:
    void sweep(LtiSystem & plant, const std::vector<double> & frequencies);

    TemplateEngine m_processor;
    const CancellationToken * m_cancellation = nullptr;
    ParameterGrids m_grids;
    bool m_borderSweep = false;
    std::shared_ptr<const DeviceClouds> m_resident;
};

PlantModel modelOf(LtiSystem & plant, const SweepPlan & plan);

SweepGrid gridOf(const SweepPlan & plan);

}

#endif
