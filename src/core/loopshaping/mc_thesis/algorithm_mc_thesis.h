#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include <cstdint>
#include <complex>
#include <optional>

#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/depth_accounting.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/common/mc_search_node.h"
#include "src/core/loopshaping/common/stages.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common/common_functions.h"

/**
 * @file
 * @brief Algorithm MC of the QFTbx thesis: the NT/NK branch and bound with
 * every strategy of chapter 4, assembled as the pseudocode of chapter 5
 * prescribes.
 *
 * - QSInv (5.1.1): the certainly infeasible subranges of every parameter
 *   are cut with NK's magnitude equations and the phase equations of sec.
 *   4.1.2 (quick_solution.h).
 * - QSFact (5.1.2): the certainly feasible subranges, per frequency, with
 *   the same equations at the opposite corner and boundary extreme; the
 *   subrange feasible at every frequency is split off as a feasible node,
 *   and the per-frequency thresholds (MM/MF) feed the tree bisection. It
 *   runs only when MG finds nothing.
 * - MG (5.2): the best gain with the other parameters at the corner of
 *   largest controller magnitude, intersecting the per-frequency feasible
 *   gain thresholds: a point solution that feeds the prune variable C
 *   (5.4.3).
 * - Tree bisection (5.3): the box is split at the stored threshold that
 *   covers the largest fraction of its range, and the feasible child is
 *   marked feasible for that frequency (the node history).
 * - Execution stages (4.4): INICIAL, area bisection, until no projected box
 *   spans the whole phase width; INTERMEDIA, tree bisection, until a full
 *   pass of the cuts produces nothing; FINAL, cuts off, bisection by the
 *   wider of magnitude and phase.
 *
 * Each strategy has its own switch (Strategies, from the research
 * settings), all on by default, for the case studies of chapter 6; none
 * changes the answer, since each discards only boxes it has certified.
 *
 * MG's gain and the feasible nodes pass the nominal stability criterion
 * (NominalStabilityChecker), and MG's candidate the feasibility test before
 * it may prune; an ambiguous box whose members are all unstable is
 * discarded, as in NT; and a standing MG solution is returned when the
 * list empties. Where the thesis is inconsistent the equations follow the
 * sound reading: B_max where sec. 4.1.1 writes B_min, the corner point and
 * not the box as MG's certified result, and the fixed corner of QSInv as
 * the one that minimises the phase. The cancellation token has to outlive
 * solve().
 */
namespace qftbx {

class AlgorithmMcThesis
{
public:

    struct Strategies {
        bool infeasibleMagnitude = true;
        bool infeasiblePhase = true;
        bool feasibleMagnitude = true;
        bool feasiblePhase = true;
        bool bestGain = true;
        bool treeBisection = true;
        bool stages = true;
    };

    void setStrategies(const Strategies & s);

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                   double epsilon);

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings);

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:

    struct FeasibleThreshold {
        std::int32_t parameter;
        std::size_t freqIndex;
        double threshold;
        bool upperSide;
        double fraction;
    };

    struct NodeAnalysis {
        std::vector<std::optional<BoxClassification>> classification;
        std::vector<std::optional<NicholsBox>> projection;
        std::vector<Range> boxMag;
        std::vector<Range> boxPhase;
        qftbx::BoxFlag flag = qftbx::feasible;
        std::size_t mainFrequency = 0;
        bool anyFullPhaseWidth = false;
    };

    bool analyse(McSearchNode * node, NodeAnalysis & out);
    bool isEpsilonSmall(McSearchNode * node, const NodeAnalysis & analysis);
    void improveNode(McSearchNode * node, NodeAnalysis & analysis,
                            std::vector<FeasibleThreshold> & thresholds);
    bool bestGainSearch(McSearchNode * node, const NodeAnalysis & analysis);
    void feasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                             std::vector<FeasibleThreshold> & thresholds, bool & improved);
    void infeasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                               bool & improved);

    qftbx::McBisectionResult bisect(McSearchNode * node, const NodeAnalysis & analysis,
                                        const std::vector<FeasibleThreshold> & thresholds);
    qftbx::McBisectionResult bisectAt(McSearchNode * node, std::int32_t parameter, double point);
    inline std::int32_t widestByMeasure(McSearchNode * node, std::size_t mainFrequency, int measure);

    bool boxIsFeasibleAt(LtiSystem * box, std::size_t freqIndex);
    bool boxIsFeasible(LtiSystem * box);
    bool pointIsFeasible(const PointController & point);
    void insertFeasibleBox(std::unique_ptr<LtiSystem> box, McSearchNode * parent);

    inline std::int32_t parameterCount(LtiSystem * box) const;
    Range parameterRange(LtiSystem * box, std::int32_t parameter) const;
    std::unique_ptr<LtiSystem> replaceParameter(LtiSystem * box, std::int32_t parameter,
                                                       Range range) const;

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

    Strategies strategies;
    DepthAccounting depthAccounting;

    double phaseGridStep = 0;
    double phaseSpanWidth = 0;

    bool hasUncertainZeros = false;
    bool hasUncertainPoles = false;

    const qftbx::CancellationToken * m_cancellation = nullptr;

    qftbx::Settings m_settings;

};

}

#endif
