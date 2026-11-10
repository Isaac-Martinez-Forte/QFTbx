/**
 * @file
 * @brief What a template engine does, on whichever processor it runs.
 *
 * The template stage drives its engine through TemplateComputation, so the
 * engine of the processor (TemplateEngine) and that of the GPU are
 * interchangeable and give the same templates. compute() sweeps the family
 * and contours every cloud, at the epsilon given or, given none, at the one
 * proposeEpsilon() finds; computeContours() contours the clouds again at a
 * new epsilon, and setClouds() hands over clouds swept elsewhere.
 * EpsilonProposal and ContourReport are the same for every engine: the
 * proposal for one cloud, and what the contour of one design frequency went
 * through.
 */

#ifndef QFTBX_TEMPLATE_COMPUTATION_H
#define QFTBX_TEMPLATE_COMPUTATION_H

#include <cstddef>
#include <vector>

#include "src/core/pipeline/cancellation.h"
#include "src/core/system/lti_system.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/parameter_grids.h"

namespace qftbx {

struct EpsilonProposal
{
    double connected = 0.0;
    double epsilon = 0.0;
    double diameter = 0.0;
    bool closes = false;
    double coarseness() const { return diameter > 0.0 ? connected / diameter : 0.0; }
};

struct ContourReport
{
    std::size_t cloudPoints = 0;
    std::size_t contourPoints = 0;
    bool relaxed = false;
    bool truncated = false;
    bool wholeCloud = false;
    std::size_t components = 1;
    std::vector<std::size_t> componentStarts;
    bool unbounded = false;
    double resonanceFrom = 0.0;
    double resonanceTo = 0.0;
};

class TemplateComputation
{
public:
    virtual ~TemplateComputation() = default;

    virtual void setCancellation(const CancellationToken * token) = 0;
    virtual void setHullMetric(HullMetric metric, double dbPerDegree) = 0;
    virtual void setWholeCloudStandsIn(bool standsIn) = 0;
    virtual void setAlphaShapeContour(bool alphaShape) = 0;
    virtual void setBorderSweep(bool border) = 0;
    virtual void setGrids(ParameterGrids grids) = 0;
    virtual void setEpsilon(std::vector<double> epsilon) = 0;
    virtual void setClouds(CloudSet clouds) = 0;

    virtual bool compute(LtiSystem * plant, std::vector<double> * frequencies) = 0;
    virtual CloudSet computeClouds(LtiSystem * plant, std::vector<double> * frequencies) = 0;
    virtual bool computeContours(std::vector<double> epsilon) = 0;
    virtual std::vector<EpsilonProposal> proposeEpsilon() = 0;

    virtual const CloudSet & clouds() const = 0;
    virtual const CloudSet & contours() const = 0;
    virtual const std::vector<double> & epsilon() const = 0;
    virtual const std::vector<ContourReport> & contourReports() const = 0;
};

}

#endif
