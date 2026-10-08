#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MR_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MR_H

#include "src/core/specifications/specification_record.h"
#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/templates/cloud_set.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include <complex>

#include <map>

#include <string>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/math/expression_tree.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common/common_functions.h"

/**
 * @file
 * @brief Algorithm MR: QFT synthesis as an interval constraint
 * satisfaction problem, solved by branch and prune.
 *
 * Rambabu Kalla and Nataraj, "Synthesis of fractional-order QFT
 * controllers using interval constraint satisfaction technique", FDA 2010.
 * The specifications become quadratic inequalities in the controller's
 * magnitude g and phase phi, one per template representative
 * p e^{j theta} and design frequency, and one per ordered pair of
 * representatives for the tracking spread (eqs. (10) and (11) of the
 * paper; eq. (9), a square bounded below by zero, never contracts and is
 * left out). Every box is narrowed by the HC4 hull-consistency filter
 * (expression_tree) to a fixpoint, an emptied domain discards it, and
 * otherwise the widest variable is bisected. No Nichols boundaries are
 * needed: the constraints come from the specifications and from a handful
 * of representatives of each template contour. The constraint trees are
 * parsed once and bound to the order of the uncertain parameters, so a box
 * is loaded without a name lookup.
 *
 * The live list is ordered by ascending gain infimum, which reaches first
 * the box the paper's sort of the solutions would pick, and the search
 * stops at the first certainly feasible box or at the first whose
 * controller parameters are all narrower than epsilon. That is the
 * paper's epsilon, on the parameter box, where the other algorithms
 * measure it on the Nichols box, so the same number means different
 * things; with algorithms.mr-nichols-epsilon MR stops on the Nichols box
 * like the others. The returned point passes the nominal stability
 * criterion (NominalStabilityChecker). The cancellation token has to
 * outlive solve().
 */
namespace qftbx {

class AlgorithmMr
{
public:

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                   double epsilon, const qftbx::CloudSet & temp,
                   const qftbx::SpecificationRecords * specificationRecords);

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:

    void buildControllerExpressions();
    void buildConstraints();
    void classifyAndInsert(std::unique_ptr<LtiSystem> box);
    bool narrowToFixpoint(std::vector<Interval> & domains);
    bool certainlyFeasible(std::vector<Interval> & domains);
    void loadDomains(LtiSystem * box, std::vector<Interval> & domains);

    void bindConstraints();

    bool isParameterBoxSmall(LtiSystem * box) const;

    void loadPointDomains(LtiSystem * box, bool lowerCorner,
                                 std::vector<Interval> & domains);
    std::unique_ptr<LtiSystem> boxFromDomains(LtiSystem * box,
                                      const std::vector<Interval> & domains);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector<double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    double epsilon = 0;
    qftbx::CloudSet temp;
    const qftbx::SpecificationRecords * specificationRecords = nullptr;

    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<OrderedList> liveList;

    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::vector<std::complex<double>> nominalPlantValues;

    std::vector<Expression> magnitudeExpressions;
    std::vector<Expression> phaseExpressions;
    std::vector<std::unique_ptr<qftbx::ExpressionTree>> constraints;

    std::vector<std::string> parameterNames;

    std::unique_ptr<LtiSystem> designedController;

    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;

};

}

#endif
