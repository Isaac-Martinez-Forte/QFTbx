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
        /// The relaxed walk hit its step limit too, so neither walk closed:
        /// the FULL CLOUD stands in for the contour at this frequency (the
        /// relaxed walk used to hand on the partial contour it had).
        bool truncated = false;
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
    bool m_useCuda = false;

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

    /// The faithful walk over one epsilon-connected set of points, with the
    /// relaxed fallback over 'fallback' (the same points, in the order the
    /// fallback has always received them) when it does not close.
    ComplexCloud walkComponent(const ComplexCloud & cv, const ComplexCloud & fallback, double epsilon,
                               const NeighbourGrid & neighbours, bool * fellBack, bool * truncated);

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
    ComplexCloud epsilonHullRelaxed(const ComplexCloud & cloud, double epsilon,
                                    bool * truncated = nullptr);

};

} // namespace qftbx


#endif // QFTBX_TEMPLATE_ENGINE_H
