#ifndef QFTBX_TEMPLATE_ENGINE_H
#define QFTBX_TEMPLATE_ENGINE_H

#include <cstdint>
#include <complex>
#include <limits>

#include <string>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/templates/parameter_grids.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/system/parameter.h"

namespace qftbx {

/**
 * @brief Computes QFT templates (plant value sets) and their contours.
 *
 * For each design frequency the brute-force sweep evaluates the plant over
 * the cartesian product of the uncertain-parameter grids, and the contour is
 * extracted with the
 * \f$\varepsilon\f$-hull algorithm (Nordin 1993, Montoya's EPSHULL.M
 * implementation): starting from the rightmost point, the walk repeatedly
 * picks the neighbour within \f$\varepsilon\f$ whose circle of radius
 * \f$\varepsilon/2\f$ sticks out of the covered region (minimum
 * \f$\psi\f$ angle), closing when it returns to the initial pair.
 *
 * Known limitation of the reference algorithm, found while porting: on
 * clouds of clusters spaced about \f$\varepsilon\f$ apart the walk cycles
 * without closing; epsilonHull() then falls back to the relaxed historical
 * walk (a valid \f$\varepsilon\f$-cover, not the canonical hull) with a
 * warning.
 *
 * The engine keeps its own copies of everything it is given: the grids,
 * the epsilons, the clouds (see the setters) and the frequencies.
 */
class TemplateEngine
{
public:
    /// Sweeps the plant and extracts every contour. Throws qftbx::Exception
    /// on invalid input or when a computation fails.
    bool compute(LtiSystem *plant, std::vector<double>* frequencies, bool cuda);

    /// Recomputes only the contours (one epsilon per frequency) over the
    /// current clouds.
    bool computeContours (std::vector <double> epsilon);

    /// Brute-force sweep: one cloud per frequency, the cartesian product of
    /// the parameter grids evaluated at s = j*omega.
    CloudSet computeClouds(LtiSystem *plant, std::vector<double>* frequencies);

    bool computeContourSet(bool cuda);

    /**
     * @brief Epsilon-hull contour of a point cloud, faithful to EPSHULL.M:
     * unique()d input in MATLAB complex order, max-real starting point, the
     * previous point stays a candidate (spikes are traversed both ways) and
     * the returned contour is closed (last point repeats the first).
     *
     * Returns empty when no candidate lies within epsilon of the start;
     * when the reference walk cycles, falls back to the relaxed historical
     * walk (open, deduplicated, max-imaginary start).
     *
     * @param cloud the plant value set at one design frequency.
     * @param epsilon how far the hull may cut across the cloud: the walk
     * guarantees every point is covered within this distance.
     * @param fellBack when not null, set to true if the faithful walk did
     * not close and the relaxed historical walk was used instead. Reported
     * by the CALLER, after the parallel loop: warning from inside an OpenMP
     * region raced on the message handler (helgrind), and it is the same
     * non-local action from within a parallel region that once let an
     * expression error terminate the process.
     */
    ComplexCloud epsilonHull(const ComplexCloud & cloud, double epsilon,
                             bool * fellBack = nullptr, bool * truncated = nullptr,
                             std::vector<std::size_t> * componentStarts = nullptr);

    /**
     * @brief The plane the epsilon of the hull is measured in.
     *
     * The walk of the hull only ever asks how far apart two points are, so
     * the plane those distances are taken in is a choice - and it decides
     * whether one epsilon can serve a whole template. In the COMPLEX plane
     * (the historical reading, and what EPSHULL.M did) an epsilon is a
     * distance in the units of the plant's response, so it means one thing
     * where the template sits at 40 dB and another where it sits at -40 dB;
     * measured on example 2 the epsilon a template needs varies by a factor
     * of ten thousand across its six frequencies, and the spacing of the
     * points by a factor of seven hundred within one template. In the
     * NICHOLS plane the distance is taken in degrees and decibels, one
     * decibel weighed against so many degrees, which is how the reference
     * implementation of the walk measures (Nordin's prune.m: an accuracy in
     * degrees and one in dB); there the epsilon a template needs varies by
     * a factor of three across those same frequencies, and the spacing by
     * a factor of thirteen. The plane is a property of the project, kept
     * with its epsilon, since the two are meaningless apart.
     */
    using HullMetric = qftbx::HullMetric;

    /// The metric and, for the Nichols plane, how many decibels weigh as
    /// much as one degree. Complex plane by default: the historical
    /// reading, and what every stored project predating the choice used.
    void setHullMetric(HullMetric metric, double dbPerDegree = 1.0);
    HullMetric hullMetric() const { return m_metric; }
    double dbPerDegree() const { return m_dbPerDegree; }

    /**
     * @brief The epsilon a cloud asks for, and how coarse the cloud is.
     *
     * The smallest epsilon that keeps a cloud connected is the longest edge
     * of its Euclidean minimum spanning tree (the last merge of single
     * linkage), in the plane of the metric. It is the least epsilon that
     * loses no point of the cloud, and being the least it is also the one
     * that rolls over the fewest concavities, so it answers both halves of
     * "which epsilon?" at once. Alongside it, the cloud's diameter in the
     * same plane: their ratio is how large the biggest gap in the sample is
     * against the size of the template, a dimensionless measure of how
     * coarse the sweep is - on example 2 with 25 points per parameter it is
     * a fifth of the template at one frequency.
     */
    struct EpsilonProposal
    {
        /// The longest edge of the minimum spanning tree: below it the cloud
        /// splits into more than one component, so no smaller epsilon can
        /// keep every point in reach of the contour.
        double connected = 0.0;
        /// The epsilon to use: the least value, on the ladder of three-figure
        /// numbers rising one per cent at a time from `connected`, at which
        /// the contour walk closes. Equal to `connected` when nothing up to
        /// the diameter closes (then `closes` is false).
        double epsilon = 0.0;
        double diameter = 0.0;   ///< the largest distance between two points
        bool closes = false;     ///< whether the walk closes at `epsilon`
        /// The gap as a fraction of the template: the connecting epsilon
        /// over the diameter. A property of the sweep, not of the walk.
        double coarseness() const { return diameter > 0.0 ? connected / diameter : 0.0; }
    };

    /**
     * @brief One proposal per frequency of the clouds held, in the current
     * metric.
     *
     * Connectivity is necessary for the walk to close but not sufficient: the
     * walk (Prune) steps from a point to a neighbour within epsilon in a
     * given angular order, and a template whose points are just connected
     * can still leave it with no admissible next step, or send it round in
     * circles. Measured on example 2, the walk closes anywhere from exactly
     * the connecting epsilon to twice it, and not monotonically. So the
     * epsilon proposed is found by walking: candidates rise from the
     * connecting epsilon one per cent at a time, each rounded up to the three
     * significant figures a person types, and the first at which the walk
     * closes is proposed. Every candidate is tried as the user would type it,
     * so what the field shows is what has been verified. Quadratic in the
     * cloud size (Prim) plus one walk per candidate.
     */
    std::vector<EpsilonProposal> proposeEpsilon();

    /**
     * @brief What the contour of one frequency went through, as data.
     *
     * The walk has two ways of not being the canonical epsilon-hull, and
     * both used to be a line on the error stream at best: falling back to
     * the relaxed historical walk when the faithful one does not close, and
     * that walk then stopping at its step limit with a partial contour. A
     * benchmark, a test or a script has no error stream to read, so the
     * facts are kept here, one report per design frequency, in the order of
     * the clouds.
     */
    struct ContourReport
    {
        std::size_t cloudPoints = 0;
        std::size_t contourPoints = 0;
        /// The faithful walk did not close; the relaxed walk was used.
        bool relaxed = false;
        /// The relaxed walk hit its step limit too (it used to hand on the
        /// partial contour it had).
        bool truncated = false;
        /// No walk closed, and the WHOLE CLOUD stands in for the contour at
        /// this frequency (setWholeCloudStandsIn). With that off, a walk
        /// that does not close is an error naming the frequency.
        bool wholeCloud = false;
        /// The epsilon-connected components of the cloud, and where each
        /// one's contour begins in the returned vector (the first at 0). The
        /// walk of Prune is defined for an epsilon-connected set (Gutman,
        /// Nordin and Cohen 2007, section 3); a cloud with more than one
        /// component is walked once per component, and the contours are
        /// concatenated in this order.
        std::size_t components = 1;
        std::vector<std::size_t> componentStarts;
    };

    /// One report per frequency of the last contour computation.
    const std::vector<ContourReport> & contourReports() const { return m_reports; }

    /// What a contour that does not close becomes: the whole cloud at that
    /// frequency (the default, always safe and only slower) or an error
    /// naming the frequency, for the user to change the epsilon or the sweep.
    void setWholeCloudStandsIn(bool standsIn) { m_wholeCloudStandsIn = standsIn; }
    bool wholeCloudStandsIn() const { return m_wholeCloudStandsIn; }

    /// Sweep grids keyed by parameter NAME; the caller keeps ownership.
    /// Takes the grids BY VALUE: the engine owns its copy and nobody has to
    /// remember to free anything. See qftbx::ParameterGrids.
    void setGrids (ParameterGrids grids);

    /// One epsilon per frequency, by value.
    void setEpsilon (std::vector <double> epsilon);

    /// Feeds precomputed clouds (e.g. loaded from a project file) so their
    /// contours can be recomputed.
    /// Takes the clouds BY VALUE: see qftbx::CloudSet for what the pointer
    /// version cost.
    void setClouds (CloudSet clouds);

    const CloudSet & clouds() const;

    const CloudSet & contours() const;

    const std::vector <double> & omega() const;

    const std::vector <double> & epsilon () const;

private:
    /// Grid for an uncertain parameter, looked up by name; throws
    /// qftbx::InvalidInput naming the parameter when the grid is missing.
    const std::vector<double> & gridFor(const Parameter & a);

    ParameterGrids m_grids;
    //The cartesian product of the grid sizes, so size_t and not int32:
    //eight uncertain parameters on a 25-point grid is 25^8, about 1.5e11,
    //which overflows a 32-bit int - and an overflowed count does not make
    //the sweep slow, it makes it silently wrong.
    std::size_t m_combinationCount = 0;
    std::vector <double> m_epsilon;
    HullMetric m_metric = HullMetric::ComplexPlane;
    double m_dbPerDegree = 1.0;
    bool m_useCuda = false;
    bool m_wholeCloudStandsIn = true;

    CloudSet m_clouds;
    CloudSet m_contours;
    std::vector<ContourReport> m_reports;
    //A copy of the frequencies compute() was given, named in the contour
    //messages. The caller's vector used to be aliased here, and the engine
    //outlives it: it is kept across a project load, which replaces the
    //project and its frequencies.
    std::vector <double> m_frequencies;

    class NeighbourGrid;

    /// The epsilon-connected components of 'cv' (sorted, deduplicated), as
    /// index lists, each ordered as in 'cv' and the components ordered by
    /// their rightmost point, so the first one holds the walk's usual seed.
    std::vector<std::vector<std::int32_t>> components(const ComplexCloud & cv, double epsilon,
                                                      const NeighbourGrid & neighbours);

    /// The points the walk measures its distances in: the points themselves
    /// in the complex plane, their phase in degrees and magnitude in
    /// decibels over dbPerDegree in the Nichols plane, the branch cut of
    /// the phase placed in the widest angular gap of the cloud so that no
    /// template is torn at -360/0.
    ComplexCloud projected(const ComplexCloud & points) const;

    /// The faithful walk over one epsilon-connected set of points. The walk
    /// measures on 'walk' and returns points of 'source', which run parallel
    /// (the same points, in the same order, in two planes); when the walk
    /// does not close the relaxed fallback runs over 'fallbackWalk' and
    /// returns points of 'fallbackSource' (the same points in the order the
    /// fallback has always received them).
    ComplexCloud walkComponent(const ComplexCloud & source, const ComplexCloud & walk,
                               const ComplexCloud & fallbackSource, const ComplexCloud & fallbackWalk,
                               double epsilon, const NeighbourGrid & neighbours,
                               bool * fellBack, bool * truncated);

    std::int32_t findSecond(std::int32_t b1, const ComplexCloud & cv, double epsilon,
                            const NeighbourGrid & neighbours);

    /// excludePrevious = true reproduces the relaxed historical variant;
    /// false is the behaviour faithful to EPSHULL.M.
    std::int32_t findNext(std::int32_t previousPoint, std::int32_t currentPoint, const ComplexCloud & cv, double epsilon,
                          const NeighbourGrid & neighbours, bool excludePrevious = false);

    /// Historical PFC walk (divergent from EPSHULL.M): max-imaginary start,
    /// previous point excluded, deduplicated output. Used as the fallback
    /// when the reference walk cycles. Empty when it hits its own step
    /// limit: it used to return the partial contour it had, silently.
    ComplexCloud epsilonHullRelaxed(const ComplexCloud & source, const ComplexCloud & walk,
                                    double epsilon, bool * truncated = nullptr);

};

} // namespace qftbx


#endif // QFTBX_TEMPLATE_ENGINE_H
