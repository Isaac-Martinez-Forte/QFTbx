#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC1_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC1_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include <complex>

#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common/common_functions.h"

/**
 * @file
 * @brief Algorithm MC of the 2021 paper: the NT/NK branch and bound
 * accelerated with the QS2 parameter box reduction.
 *
 * Martinez-Forte and Cervera, "Accelerated quantitative feedback theory
 * interval automatic loop shaping algorithm", Int. J. Robust Nonlinear
 * Control 31, 2021, DOI 10.1002/rnc.5499. QS2 adds to NK's Quick Solution
 * two sources of information:
 *
 * - Stage 2, phase: when a vertical strip of the box's Nichols rectangle
 *   is certainly forbidden, the monotonicity of every zero and pole term
 *   gives closed-form cuts (quick_solution.h), the counterpart of the
 *   magnitude cuts of stage 1, NK's Quick Solution.
 * - Stage 3, feasible boxes: the largest upper subrange [k_f, sup k] of the
 *   gain whose box is certainly feasible at every design frequency gives a
 *   certified solution with gain k_f, which feeds the prune variable C of
 *   step 3bis: a box whose gain cannot improve C is discarded, and every
 *   new box's gain is capped at C.
 *
 * The feasible box of the paper is kept as the certified controller behind
 * C, returned when the list is exhausted without anything better, and the
 * capped boxes are the remainder: the same prune, with no duplicate list
 * entries. Stage 3 finds k_f by logarithmic bisection over the feasibility
 * test of the gain range [k, k_max], which shrinks as k grows, so the test
 * is monotonic in k and the bisection finds k_f within its tolerance. The
 * returned point passes the nominal stability criterion, and an ambiguous
 * box whose members are all unstable is discarded, as in NT. The
 * cancellation token has to outlive solve().
 */
namespace qftbx {

class AlgorithmMc1
{
public:

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                   double epsilon);

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings) { m_settings = settings; }

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:

    void check_box_feasibility(std::unique_ptr<LtiSystem> box);
    std::unique_ptr<LtiSystem> quickSolution2(std::unique_ptr<LtiSystem> v,
                                                    const BoxClassification & classification,
                                      const NicholsBox & projection, double w,
                                      std::complex<double> p0);
    void certifiedGainSearch(LtiSystem * box);
    bool gainRangeIsFeasible(const std::vector<NaturalIntervalExtension::Factors> & factors,
                             double gainInf, double gainSup);

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector<double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    double epsilon = 0;

    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<BoundaryViolationDetector> detector;
    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<FamilyStabilityChecker> family;
    qftbx::ParameterGrids m_sweep;
    std::unique_ptr<OrderedList> liveList;
    std::vector<std::complex<double>> nominalPlantValues;

    double bestCertifiedGain = 0;
    std::unique_ptr<LtiSystem> bestCertifiedController;

    std::unique_ptr<LtiSystem> designedController;

    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;

};

}

#endif
