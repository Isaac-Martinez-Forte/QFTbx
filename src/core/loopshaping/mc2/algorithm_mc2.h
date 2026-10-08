#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC2_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC2_H

#include <complex>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/common/boundary_violation_detector.h"
#include "src/core/loopshaping/common/certifier.h"
#include "src/core/loopshaping/common/gain_contractor.h"
#include "src/core/loopshaping/common/common_functions.h"
#include "src/core/loopshaping/common/depth_accounting.h"
#include "src/core/loopshaping/common/exact_point_check.h"
#include "src/core/loopshaping/common/family_stability_checker.h"
#include "src/core/loopshaping/common/mc_search_node.h"
#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/loop_shaping_statistics.h"
#include "src/core/math/range_union.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/project/settings.h"
#include "src/core/system/lti_system.h"

/**
 * @file
 * @brief Algorithm MC2: the strategies of the QFTbx thesis with the errors
 * of their formulation corrected, and a search that certifies its answer.
 *
 * The thesis (Martinez-Forte 2022, chapters 4 and 5) adds to the NT/NK
 * interval branch and bound four cutting strategies, a best-gain search, a
 * tree bisection and execution stages. Its formulation has errors: the
 * cuts read B_min for B_max and the wrong vertices, and the best gain can
 * violate the boundaries. MC of the thesis (algorithm_mc_thesis) repairs
 * them one by one; MC2 rebuilds the formulation:
 *
 * - T1: the four cuts are valid only with the other parameters at one
 *   vertex, and they use two vertices, cross-paired: infeasible magnitude
 *   with feasible phase, feasible magnitude with infeasible phase.
 * - T2: a certified subrange is cut off as a slab, so what is left is a box.
 * - T3: with the zeros and poles fixed the phase does not depend on the
 *   gain, so one phase column of each frequency binds it, and the
 *   admissible gains are a finite union of intervals (RangeUnion),
 *   intersected over the frequencies. No equation is solved, a frequency
 *   that admits nothing empties the set on its own, and the smallest
 *   admissible gain may lie in the lower branch of a closed boundary. The
 *   best gain of a box is that of its vertex of largest magnitude, which is
 *   that of smallest phase.
 * - P2: with the vertex of T1 the angle of the phase equation is below 90
 *   degrees, so its check is an invariant.
 *
 * The execution stages are left out: without them the search is faster and
 * the gain no higher. Their bisection rule, the parameter that most narrows
 * the wider side of the projection, is used everywhere. The tree bisection,
 * the node history and the search node (common/mc_search_node.h) are those
 * of MC of the thesis.
 *
 * Under the exact reading, the default, a point becomes a design only
 * through the certification funnel (common/certifier.h), with the smallest
 * gain the specifications admit at its zeros and poles
 * (ExactPointCheck::lowestAdmissibleGain). The search ends as a branch and
 * bound ends: a certified point becomes the best design so far and prunes
 * every box that cannot beat it, a tie included, until the list is empty.
 * Every box taken has its gain contracted with proofs first
 * (GainContractor); a box the columns call infeasible is discarded only
 * when the exact sector verdict proves it, whose strips are also the
 * magnitude cuts, and the phase cuts are left out; a box the nominal
 * criterion finds unstable on its grid is discarded only when the Routh
 * table or the zero exclusion proves it. What is left without a proof, a
 * box resolved at the epsilon size or dropped with no certified point, is
 * counted with the smallest gain it could hold
 * (LoopShapingStatistics::Certificate), and "no feasible solution" is
 * claimed only when nothing was. The widest parameter is measured without
 * the nominal plant, and a parameter that is a point is not split.
 *
 * The cancellation token, the templates and the specifications have to
 * outlive solve(); without templates that cover every design frequency the
 * search reads the columns, and without the grids of the sweep it leaves
 * the family out.
 *
 * research.mc2.gain-contraction = 0 gives the search without the
 * contraction, with the family gate (FamilyStabilityChecker::isBoxUnstable)
 * and the grid's discards counted apart; research.mc2-reading = columns
 * gives the published formulation, bit for bit, with the certificate as
 * bookkeeping alone.
 */
namespace qftbx {

class AlgorithmMc2
{
public:

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                   double epsilon);

    void setCancellation(const qftbx::CancellationToken * token)
    { m_cancellation = token; }

    void setSettings(const qftbx::Settings & settings);

    void setPlantFamily(qftbx::ParameterGrids sweep) { m_sweep = std::move(sweep); }

    void setSpecifications(const qftbx::CloudSet * templates, const qftbx::SpecificationSet * specifications)
    { m_templates = templates; m_specifications = specifications; }

    bool solve();

    std::unique_ptr<LtiSystem> controllerStructure();

    LoopShapingStatistics statistics() const;

private:

    struct FeasibleThreshold {
        std::int32_t parameter;
        std::size_t freqIndex;
        double threshold;
        bool upperSide;
    };

    struct NodeAnalysis {
        std::vector<std::optional<BoxClassification>> classification;
        std::vector<std::optional<NicholsBox>> projection;
        qftbx::BoxFlag flag = qftbx::feasible;
        std::size_t mainFrequency = 0;
    };

    enum class Step { Next, Carry, Designed };
    enum class WidthMeasure { Area, Magnitude, Phase };

    void prepare();
    bool concludeEmptyList();
    Step returnDesign(std::unique_ptr<LtiSystem> design);
    Step resolveFeasibleHead(McSearchNode & node);
    bool analyseOrDiscard(McSearchNode & node, NodeAnalysis & analysis, double gainInf);
    Step resolveFeasibleCorner(McSearchNode & node, double gainInf);
    Step resolveEpsilonBox(McSearchNode & node, double gainInf);
    bool pruneUnstableBox(McSearchNode & node, double gainInf);
    void expand(McSearchNode & node, NodeAnalysis & analysis);

    void discardUnproven(double gainInf);
    void discardGridBacked(double gainInf);
    void dropToResidue(double gainInf);
    void resolvedAtEpsilon(double gainInf);
    void adoptIncumbent(const PointController & design, LtiSystem * box);
    void closeCertificate();
    bool cannotImprove(double gainInf) const;

    bool analyse(McSearchNode * node, NodeAnalysis & out);
    bool isEpsilonSmall(McSearchNode * node, const NodeAnalysis & analysis);
    bool improveNode(McSearchNode * node, NodeAnalysis & analysis, std::vector<FeasibleThreshold> & thresholds);

    RangeUnion columnGainsDb(const std::vector<double> & zeros, const std::vector<double> & poles, Range gainRange);

    bool bestGainSearch(McSearchNode * node);
    std::optional<double> lowestGain(const std::vector<double> & zeros, const std::vector<double> & poles,
                                     Range gainRange);
    bool accepts(const PointController & point);

    void feasibleCuts(McSearchNode * node, const NodeAnalysis & analysis, std::vector<FeasibleThreshold> & thresholds);
    bool infeasibleCuts(McSearchNode * node, const NodeAnalysis & analysis);

    qftbx::McBisectionResult bisect(McSearchNode * node, const NodeAnalysis & analysis,
                                    const std::vector<FeasibleThreshold> & thresholds);
    qftbx::McBisectionResult bisectAt(McSearchNode * node, std::int32_t parameter, double point);
    inline std::int32_t widestByMeasure(McSearchNode * node, std::size_t mainFrequency, WidthMeasure measure);

    bool boxIsFeasibleAt(LtiSystem * box, std::size_t freqIndex);
    bool boxIsFeasible(LtiSystem * box);
    void insertFeasibleBox(std::unique_ptr<LtiSystem> box);

    std::optional<PointController> bestEpsilonCandidate(LtiSystem * box);
    PointController lowestGainOnRay(const PointController & point);

    inline std::int32_t parameterCount(LtiSystem * box) const;
    Range parameterRange(LtiSystem * box, std::int32_t parameter) const;
    std::unique_ptr<LtiSystem> replaceParameter(LtiSystem * box, std::int32_t parameter, Range range) const;

    LtiSystem * plant = nullptr;
    std::unique_ptr<LtiSystem> controller;
    std::vector<double> * omega = nullptr;
    const BoundaryData * boundaries = nullptr;
    double epsilon = 0;
    const qftbx::CloudSet * m_templates = nullptr;
    const qftbx::SpecificationSet * m_specifications = nullptr;
    qftbx::ParameterGrids m_sweep;
    qftbx::Settings m_settings;
    const qftbx::CancellationToken * m_cancellation = nullptr;

    std::unique_ptr<NaturalIntervalExtension> conversion;
    std::unique_ptr<BoundaryViolationDetector> detector;
    std::unique_ptr<NominalStabilityChecker> stability;
    std::unique_ptr<FamilyStabilityChecker> family;
    std::unique_ptr<ExactPointCheck> exact;
    std::unique_ptr<Certifier> certifier;
    std::unique_ptr<GainContractor> contractor;

    bool exactReading = false;
    bool gainContraction = false;
    Settings::Research::McStrategies strategies;
    double phaseGridStep = 0;
    Range initialGainRange;
    std::vector<std::complex<double>> nominalPlantValues;
    std::unique_ptr<OrderedList> liveList;
    double bestCertifiedGain = std::numeric_limits<double>::infinity();
    std::unique_ptr<LtiSystem> bestCertifiedController;
    std::unique_ptr<LtiSystem> designedController;
    DepthAccounting depthAccounting;

    LoopShapingStatistics::Certificate certificate;
    double residueGainInf = std::numeric_limits<double>::infinity();
    double unprovenGainInf = std::numeric_limits<double>::infinity();
    double gridBackedGainInf = std::numeric_limits<double>::infinity();
    double resolvedGainInf = std::numeric_limits<double>::infinity();
};

}

#endif
