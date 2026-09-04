#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MR_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MR_H

#include "src/core/specifications/specification_record.h"
#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/templates/cloud_set.h"
#include <complex>

#include <map>

#include <string>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/expression_tree.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common_functions.h"

/**
 * @brief Algorithm MR: QFT synthesis as an interval constraint
 * satisfaction problem, solved by branch & prune.
 *
 * Rambabu Kalla and Nataraj, "Synthesis of fractional-order
 * QFT controllers using interval constraint satisfaction technique",
 * FDA 2010): QFT synthesis as an interval constraint satisfaction
 * problem. The specifications translate into quadratic inequalities in
 * the controller magnitude g and phase phi, one per plant template
 * representative p e^{j theta} and design frequency (and one per ORDERED
 * representative pair for the tracking spread), eqs. (9)-(11) of the
 * paper. The ICSP is solved by branch & prune: every box is narrowed by
 * the HC4 hull-consistency filter (expression_tree) over the
 * constraint set to a fixpoint; an emptied domain discards the box, and
 * the search bisects the widest variable otherwise. Unlike NT/NK, no
 * Nichols boundaries are needed: the constraints come straight from the
 * specifications and the templates.
 *
 * QFTbx deviations, documented:
 * - The live list is ordered by ascending gain infimum and the search
 *   stops at the first certainly feasible box (every constraint's
 *   interval evaluation non-negative) or at the epsilon-small leading box,
 *   where epsilon-small means what it means in the paper: the width of the
 *   CONTROLLER PARAMETER box (see isParameterBoxSmall). The other four
 *   algorithms measure epsilon on the NICHOLS box instead, because that is
 *   the criterion of their own papers, so the same number means different
 *   things depending on the algorithm picked - the note on setProblem says
 *   what. The paper collects all solution boxes of that width and sorts
 *   them afterwards; ordering the live list by gain infimum reaches the
 *   minimum-gain one first, which is the one the sort would pick.
 * - The template contour is subsampled to a handful of representatives
 *   per frequency (the paper uses 9 plants; the full contour would square
 *   into the tracking pairs).
 * - The paper's eq. (9) (plain robust stability) is a square lower-bounded
 *   by zero: it never contracts anything and is omitted.
 * - The returned point must pass the nominal closed-loop stability
 *   criterion (NominalStabilityChecker).
 */
namespace qftbx {

class AlgorithmMr
{
public:

    /**
     * @brief Publishes the problem.
     *
     * @param plant the nominal plant, for the nominal stability check.
     * @param controller the initial search box of the controller
     * parameters; the algorithm takes it over.
     * @param omega the design frequencies.
     * @param temp the plant template contour at each of them, from which
     * the constraint representatives are drawn.
     * @param specificationRecords the specifications the constraints are built from.
     * @param epsilon termination width of the CONTROLLER PARAMETER box, as
     * in the paper (its eps = 0.001 on the boxes of section 5). This is NOT
     * the epsilon of the other four algorithms, which measure the diameter
     * of the Nichols box: on a plant whose |P| reaches 1e4 at the lowest
     * design frequency, a Nichols epsilon of 0.001 would demand a gain
     * interval narrower than 1e-7, and the two numbers are neither
     * comparable nor interchangeable.
     * @param boundaries unused: the constraints come from the
     * specifications and the templates, not from Nichols boundaries.
     */
    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                   double epsilon, const qftbx::CloudSet & temp,
                   const qftbx::SpecificationRecords * specificationRecords);

    /**
     * @brief Installs the flag the search reads once per node.
     *
     * A pointer, and null by default: a caller that never cancels - every
     * test that drives this algorithm directly - carries on unchanged. The
     * token has to outlive solve().
     */
    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    /**
     * @brief The values the user may have changed.
     *
     * The whole struct rather than one setter per value: what an algorithm
     * needs from it is copied here, once, before solve() - so the hot path
     * reads a member and never a configuration lookup. Not calling it leaves
     * the compiled defaults, which is what every existing caller does.
     */
    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    bool solve();

    /// The designed controller, handed over to the caller.
    std::unique_ptr<LtiSystem> controllerStructure();

    /// The most boxes the search kept alive at once (see kDefaultMaxLiveNodes).
    std::size_t peakLiveNodes() const;

private:

    void buildControllerExpressions();
    void buildConstraints();
    void classifyAndInsert(std::unique_ptr<LtiSystem> box);
    bool narrowToFixpoint(std::map<std::string, cxsc::interval> & domains);
    bool certainlyFeasible(std::map<std::string, cxsc::interval> & domains);
    void loadDomains(LtiSystem * box, std::map<std::string, cxsc::interval> & domains);

    /// True when every uncertain controller parameter has been narrowed to
    /// an interval no wider than epsilon: the paper's termination criterion.
    bool isParameterBoxSmall(LtiSystem * box) const;

    /// Degenerate domains at the corner pointFromBox() would take, so that
    /// the point itself can be run through the constraint set.
    void loadPointDomains(LtiSystem * box, bool lowerCorner,
                                 std::map<std::string, cxsc::interval> & domains);
    std::unique_ptr<LtiSystem> boxFromDomains(LtiSystem * box,
                                      const std::map<std::string, cxsc::interval> & domains);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector<double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    double epsilon = 0;
    qftbx::CloudSet temp;
    const qftbx::SpecificationRecords * specificationRecords = nullptr;

    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<OrderedList> liveList;

    //Controller magnitude/phase expression strings, one per design
    //frequency, and the parsed constraint trees (built once; each box
    //only reloads the variable domains).
    std::vector<std::string> magnitudeExpressions;
    std::vector<std::string> phaseExpressions;
    std::vector<std::unique_ptr<qftbx::ExpressionTree>> constraints;
    //The source text of each constraint, for diagnostics.
    std::vector<std::string> constraintTexts;

    std::unique_ptr<LtiSystem> designedController;


    /// Not owned. Null means this run cannot be cancelled.
    const qftbx::CancellationToken * m_cancellation = nullptr;

    /// Copied whole and read as fields; the defaults are the compiled ones.
    qftbx::Settings m_settings;

};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MR_H
