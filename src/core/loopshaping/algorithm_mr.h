#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MR_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MR_H

#include "src/core/templates/cloud_set.h"
#include <complex>

#include <map>

#include <QString>
#include <QVector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/expression_tree.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common_functions.h"

/*
 * Algorithm MR (Rambabu Kalla and Nataraj, "Synthesis of fractional-order
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
 *   things depending on the algorithm picked - the note on set_datos says
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
class AlgorithmMr
{
public:
    AlgorithmMr();
    ~AlgorithmMr();

    /**
     * @brief Publishes the problem.
     *
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
    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega, const BoundaryData * boundaries,
                   qreal epsilon, const qftbx::CloudSet & temp,
                   const qftbx::SpecificationRecords * espe);

    bool init_algorithm();

    /// The designed controller, handed over to the caller.
    std::unique_ptr<LtiSystem> controllerStructure();

    /// The most boxes the search kept alive at once (see kDefaultMaxLiveNodes).
    std::size_t peakLiveNodes() const;

private:

    struct BoxDomains {
        std::map<std::string, cxsc::interval> values;
    };

    inline void buildControllerExpressions();
    inline void buildConstraints();
    inline void classifyAndInsert(std::unique_ptr<LtiSystem> box);
    inline bool narrowToFixpoint(std::map<std::string, cxsc::interval> & domains);
    inline bool certainlyFeasible(std::map<std::string, cxsc::interval> & domains);
    inline void loadDomains(LtiSystem * box, std::map<std::string, cxsc::interval> & domains);

    /// True when every uncertain controller parameter has been narrowed to
    /// an interval no wider than epsilon: the paper's termination criterion.
    inline bool isParameterBoxSmall(LtiSystem * box) const;

    /// Degenerate domains at the corner pointFromBox() would take, so that
    /// the point itself can be run through the constraint set.
    inline void loadPointDomains(LtiSystem * box, bool lowerCorner,
                                 std::map<std::string, cxsc::interval> & domains);
    inline std::unique_ptr<LtiSystem> boxFromDomains(LtiSystem * box,
                                      const std::map<std::string, cxsc::interval> & domains);

    LtiSystem * planta = nullptr;
    std::unique_ptr<LtiSystem> controlador;
    QVector<qreal> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    qreal epsilon = 0;
    qftbx::CloudSet temp;
    const qftbx::SpecificationRecords * espe = nullptr;

    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<OrderedList> lista;

    //Controller magnitude/phase expression strings, one per design
    //frequency, and the parsed constraint trees (built once; each box
    //only reloads the variable domains).
    QVector<QString> magnitudeExpressions;
    QVector<QString> phaseExpressions;
    std::vector<std::unique_ptr<alg::ExpressionTree>> constraints;
    //The source text of each constraint, for diagnostics.
    QVector<QString> constraintTexts;

    std::unique_ptr<LtiSystem> controlador_retorno;

};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MR_H
