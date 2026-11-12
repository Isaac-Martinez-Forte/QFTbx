/**
 * @file
 * @brief Computation of QFT templates and of their epsilon-hull contours.
 *
 * The engine sweeps the plant at s = j*omega over the cartesian product of
 * the uncertain-parameter grids at each design frequency, and walks the
 * contour of each cloud with the \f$\varepsilon\f$-hull (Nordin 1993),
 * faithful to Montoya's EPSHULL.M: unique()d input in MATLAB complex
 * order, max-real start, the previous point still a candidate, the next one
 * the neighbour of least \f$\psi\f$ angle, the closed contour repeating its
 * first point, and empty when nothing lies within \f$\varepsilon\f$ of the
 * start. A walk that does not close falls back to the relaxed walk
 * (max-imaginary start, previous point excluded, open), a valid cover but
 * not the canonical hull, empty at its step limit so that a partial contour
 * never passes for a whole one. Each \f$\varepsilon\f$-connected component
 * is walked apart (Gutman, Nordin and Cohen 2007), the contours joined in
 * order of rightmost point. ContourReport records this per frequency after
 * the parallel loop, since nothing may warn or throw inside an OpenMP
 * region; a contour that never closes becomes the whole cloud, or an error
 * naming the frequency. The alpha-shape contour is the same boundary by
 * definition and always closes.
 *
 * Distances are taken in the complex plane (the default, also for a
 * project that names none) or in the Nichols plane, dbPerDegree decibels
 * weighing as one degree, with the phase cut in the widest angular gap;
 * the plane belongs to the project, with its epsilon. proposeEpsilon()
 * climbs from the longest edge of the minimum spanning tree, the least
 * epsilon keeping the cloud connected, one per cent at a time in three
 * significant figures, to the first that closes (or stays there, with
 * closes false). withoutRetracing() then tries at most four steps of 1.5
 * for a walk that passes no point twice, since the least closing epsilon
 * goes into every notch of the lattice; on a curve it returns the closing
 * one. compute() given no epsilon contours each template at its proposal,
 * made on the clouds it has just swept, so a proposal costs no second
 * sweep; the proposals are kept until the clouds, the plane or the contour
 * method change.
 *
 * With exactly two uncertain parameters the border sweep spends the same
 * budget on the edges of the box, round it along the user's grids: the
 * template's border lies in the image of the box's border plus isolated
 * critical values, and every bounded closed-loop magnitude, a Mobius
 * function of the plant, peaks there while its pole is outside. Its contour
 * is the alpha-shape at the connecting epsilon, never the walk. The sweep
 * notes a family whose plants differ in right half-plane poles
 * (poleCountChange), and marks in the report the design frequencies inside
 * a band swept by poles of its plants on the imaginary axis, where the
 * true template is unbounded and the
 * sweep only reaches as high as its plants go; the plan and the run of the
 * sweep are in sweep_plan.h. An engine that sweeps elsewhere hands its
 * clouds over with the plan (adoptSweep) and has them contoured here
 * (contourClouds). The token is read once per frequency and throws
 * qftbx::Cancelled; null cannot be cancelled. The engine keeps copies of all it is given, and the
 * combination count is a size_t: the product of grid sizes overflows 32
 * bits.
 */

#ifndef QFTBX_TEMPLATE_ENGINE_H
#define QFTBX_TEMPLATE_ENGINE_H

#include <array>
#include <cstdint>
#include "src/core/pipeline/cancellation.h"
#include <complex>
#include <limits>

#include <string>
#include <chrono>
#include <optional>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/sweep_plan.h"
#include "src/core/templates/template_computation.h"
#include "src/core/system/parameter.h"

namespace qftbx {

class TemplateEngine : public TemplateComputation
{
public:
    bool compute(LtiSystem *plant, std::vector<double>* frequencies) override;

    void setCancellation(const qftbx::CancellationToken * token) override { m_cancellation = token; }

    bool computeContours (std::vector <double> epsilon) override;

    CloudSet computeClouds(LtiSystem *plant, std::vector<double>* frequencies) override;

    void adoptSweep(CloudSet clouds, const SweepPlan & plan, std::vector<double> frequencies);

    bool contourClouds();

    bool computeContourSet();

    void logContours(std::chrono::steady_clock::time_point since) const;

    ComplexCloud epsilonHull(const ComplexCloud & cloud, double epsilon,
                             bool * fellBack = nullptr, bool * truncated = nullptr,
                             std::vector<std::size_t> * componentStarts = nullptr);

    double withoutRetracing(const ComplexCloud & cloud, double closing, double diameter);

    static bool retraces(const ComplexCloud & walked);

    using HullMetric = qftbx::HullMetric;

    void setHullMetric(HullMetric metric, double dbPerDegree = 1.0) override;
    HullMetric hullMetric() const { return m_metric; }

    void setAlphaShapeContour(bool alphaShape) override;
    bool alphaShapeContour() const { return m_alphaShape; }

    void setBorderSweep(bool border) override { m_borderSweep = border; }

    std::optional<int> familyRightHalfPlanePoles() const { return m_familyRhpPoles; }
    bool borderSweep() const { return m_borderSweep; }
    bool borderSweepApplied() const { return m_borderSweepApplied; }

    ComplexCloud alphaShapeContour(const ComplexCloud & cloud, double epsilon,
                                   std::vector<std::size_t> * componentStarts) const;

    double connectingEpsilon(const ComplexCloud & cloud) const;
    double dbPerDegree() const { return m_dbPerDegree; }

    std::vector<EpsilonProposal> proposeEpsilon() override;

    const std::vector<ContourReport> & contourReports() const override { return m_reports; }
    const std::optional<PoleCountChange> & poleCountChange() const override { return m_poleCountChange; }

    void setWholeCloudStandsIn(bool standsIn) override { m_wholeCloudStandsIn = standsIn; }
    bool wholeCloudStandsIn() const { return m_wholeCloudStandsIn; }

    void setGrids (ParameterGrids grids) override;

    void setEpsilon (std::vector <double> epsilon) override;

    void setClouds (CloudSet clouds) override;

    const CloudSet & clouds() const override;

    const CloudSet & contours() const override;

    const std::vector <double> & omega() const;

    const std::vector <double> & epsilon () const override;

private:
    const qftbx::CancellationToken * m_cancellation = nullptr;


    ParameterGrids m_grids;
    std::size_t m_combinationCount = 0;
    std::vector <double> m_epsilon;
    HullMetric m_metric = HullMetric::ComplexPlane;
    double m_dbPerDegree = 1.0;
    bool m_wholeCloudStandsIn = true;
    bool m_alphaShape = false;
    bool m_borderSweep = false;
    bool m_borderSweepApplied = false;
    std::optional<int> m_familyRhpPoles;
    std::optional<PoleCountChange> m_poleCountChange;
    std::vector<std::optional<std::array<double, 2>>> m_resonanceBands;
    std::optional<std::vector<EpsilonProposal>> m_proposals;

    CloudSet m_clouds;
    CloudSet m_contours;
    std::vector<ContourReport> m_reports;
    std::vector <double> m_frequencies;

    void adoptPlan(const SweepPlan & plan);

    class NeighbourGrid;

    std::vector<std::vector<std::int32_t>> components(const ComplexCloud & cv, double epsilon,
                                                      const NeighbourGrid & neighbours);

    ComplexCloud projected(const ComplexCloud & points) const;

    ComplexCloud walkComponent(const ComplexCloud & source, const ComplexCloud & walk,
                               const ComplexCloud & fallbackSource, const ComplexCloud & fallbackWalk,
                               double epsilon, const NeighbourGrid & neighbours,
                               bool * fellBack, bool * truncated);

    std::int32_t findSecond(std::int32_t b1, const ComplexCloud & cv, double epsilon,
                            const NeighbourGrid & neighbours);

    std::int32_t findNext(std::int32_t previousPoint, std::int32_t currentPoint, const ComplexCloud & cv, double epsilon,
                          const NeighbourGrid & neighbours, bool excludePrevious = false);

    ComplexCloud epsilonHullRelaxed(const ComplexCloud & source, const ComplexCloud & walk,
                                    double epsilon, bool * truncated = nullptr);

};

}

#endif
