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

#include "mpParser.h"

#ifdef OpenMP_AVAILABLE
    #include <omp.h>
#endif

namespace qftbx {

/**
 * @brief Computes QFT templates (plant value sets) and their contours.
 *
 * For each design frequency the brute-force sweep evaluates the plant over
 * the cartesian product of the uncertain-parameter grids (one bound
 * muParserX expression per frequency), and the contour is extracted with the
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
 * The engine owns none of the data it is given or produces: grids and
 * epsilon belong to the caller, and clouds/contours become property of the
 * template DAO as soon as the controller hands them over.
 */
class TemplateEngine
{
public:

    TemplateEngine();

    ~TemplateEngine();

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
     * non-local action from within a parallel region that once let a
     * muParserX error terminate the process.
     */
    ComplexCloud epsilonHull(const ComplexCloud & cloud, double epsilon,
                             bool * fellBack = nullptr);

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

    std::vector <double> * omega();

    const std::vector <double> & epsilon ();

private:
    /// Grid for an uncertain parameter, looked up by name; throws
    /// qftbx::InvalidInput naming the parameter when the grid is missing.
    const std::vector<double> & gridFor(Parameter & a);

    //The engine owns NOTHING below: grids and epsilon belong to the caller,
    //clouds/contours to the template DAO once handed over.
    ParameterGrids m_grids;
    std::int32_t m_combinationCount = 0;
    std::vector <double> m_epsilon;
    bool m_useCuda = false;

    CloudSet m_clouds;
    CloudSet m_contours;
    std::vector <double> * m_frequencies = NULL;

    std::int32_t findSecond(std::int32_t b1, const ComplexCloud & cv, double epsilon);

    /// excludePrevious = true reproduces the relaxed historical variant;
    /// false is the behaviour faithful to EPSHULL.M.
    std::int32_t findNext(std::int32_t previousPoint, std::int32_t currentPoint, const ComplexCloud & cv, double epsilon,
                           bool excludePrevious = false);

    /// Historical PFC walk (divergent from EPSHULL.M): max-imaginary start,
    /// previous point excluded, silent truncation at MAXP, deduplicated
    /// output. Used as the fallback when the reference walk cycles: it
    /// always yields a contour with coverage <= epsilon.
    ComplexCloud epsilonHullRelaxed(const ComplexCloud & cloud, double epsilon);

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TemplateEngine;

#endif // QFTBX_TEMPLATE_ENGINE_H
