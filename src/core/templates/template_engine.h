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
 * one.
 *
 * With exactly two uncertain parameters the border sweep spends the same
 * budget on the edges of the box, round it along the user's grids: the
 * template's border lies in the image of the box's border plus isolated
 * critical values, and every bounded closed-loop magnitude, a Mobius
 * function of the plant, peaks there while its pole is outside. Its contour
 * is the alpha-shape at the connecting epsilon, never the walk. The sweep
 * refuses a family whose plants differ in right half-plane poles. The token
 * is read once per frequency and throws qftbx::Cancelled; null cannot be
 * cancelled. The engine keeps copies of all it is given, and the
 * combination count is a size_t: the product of grid sizes overflows 32
 * bits.
 */

#ifndef QFTBX_TEMPLATE_ENGINE_H
#define QFTBX_TEMPLATE_ENGINE_H

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
#include "src/core/system/parameter.h"

namespace qftbx {

class TemplateEngine
{
public:
    bool compute(LtiSystem *plant, std::vector<double>* frequencies, bool cuda);

    void setCancellation(const qftbx::CancellationToken * token) { m_cancellation = token; }

    bool computeContours (std::vector <double> epsilon);

    CloudSet computeClouds(LtiSystem *plant, std::vector<double>* frequencies);

    bool computeContourSet(bool cuda);

    void logContours(std::chrono::steady_clock::time_point since) const;

    ComplexCloud epsilonHull(const ComplexCloud & cloud, double epsilon,
                             bool * fellBack = nullptr, bool * truncated = nullptr,
                             std::vector<std::size_t> * componentStarts = nullptr);

    double withoutRetracing(const ComplexCloud & cloud, double closing, double diameter);

    static bool retraces(const ComplexCloud & walked);

    using HullMetric = qftbx::HullMetric;

    void setHullMetric(HullMetric metric, double dbPerDegree = 1.0);
    HullMetric hullMetric() const { return m_metric; }

    void setAlphaShapeContour(bool alphaShape) { m_alphaShape = alphaShape; }
    bool alphaShapeContour() const { return m_alphaShape; }

    void setBorderSweep(bool border) { m_borderSweep = border; }

    std::optional<int> familyRightHalfPlanePoles() const { return m_familyRhpPoles; }
    bool borderSweep() const { return m_borderSweep; }
    bool borderSweepApplied() const { return m_borderSweepApplied; }

    ComplexCloud alphaShapeContour(const ComplexCloud & cloud, double epsilon,
                                   std::vector<std::size_t> * componentStarts) const;

    double connectingEpsilon(const ComplexCloud & cloud) const;
    double dbPerDegree() const { return m_dbPerDegree; }

    struct EpsilonProposal
    {
        double connected = 0.0;
        double epsilon = 0.0;
        double diameter = 0.0;
        bool closes = false;
        double coarseness() const { return diameter > 0.0 ? connected / diameter : 0.0; }
    };

    std::vector<EpsilonProposal> proposeEpsilon();

    struct ContourReport
    {
        std::size_t cloudPoints = 0;
        std::size_t contourPoints = 0;
        bool relaxed = false;
        bool truncated = false;
        bool wholeCloud = false;
        std::size_t components = 1;
        std::vector<std::size_t> componentStarts;
    };

    const std::vector<ContourReport> & contourReports() const { return m_reports; }

    void setWholeCloudStandsIn(bool standsIn) { m_wholeCloudStandsIn = standsIn; }
    bool wholeCloudStandsIn() const { return m_wholeCloudStandsIn; }

    void setGrids (ParameterGrids grids);

    void setEpsilon (std::vector <double> epsilon);

    void setClouds (CloudSet clouds);

    const CloudSet & clouds() const;

    const CloudSet & contours() const;

    const std::vector <double> & omega() const;

    const std::vector <double> & epsilon () const;

private:
    const qftbx::CancellationToken * m_cancellation = nullptr;

    const std::vector<double> & gridFor(const Parameter & a);

    ParameterGrids m_grids;
    std::size_t m_combinationCount = 0;
    std::vector <double> m_epsilon;
    HullMetric m_metric = HullMetric::ComplexPlane;
    double m_dbPerDegree = 1.0;
    bool m_useCuda = false;
    bool m_wholeCloudStandsIn = true;
    bool m_alphaShape = false;
    bool m_borderSweep = false;
    bool m_borderSweepApplied = false;
    std::optional<int> m_familyRhpPoles;

    CloudSet m_clouds;
    CloudSet m_contours;
    std::vector<ContourReport> m_reports;
    std::vector <double> m_frequencies;

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
