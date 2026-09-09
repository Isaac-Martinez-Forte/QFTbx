#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC2_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC2_H

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
#include "src/core/loopshaping/common/ordered_list.h"
#include "src/core/loopshaping/common/mc_search_node.h"
#include "src/core/loopshaping/common/stages.h"
#include "src/core/loopshaping/common/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common/common_functions.h"

/**
 * @brief Algorithm MC2: the strategies of the QFTbx thesis with the errors
 * of their formulation corrected.
 *
 * The thesis (Martinez-Forte 2022, chapters 4 and 5) extends the NT/NK
 * interval branch & bound with four cutting strategies, a best-gain search,
 * a tree bisection and the execution stages. The strategies are sound but
 * their published formulation is not: the equations of sec. 4.1.1 compare
 * against B_min where the text prescribes B_max, which makes the cut wrong
 * rather than conservative; the pseudocode of QSInv fixes the phase cut at
 * the vertex that minimises the phase where it needs the one that maximises
 * it; QSFact asks a single vertex to minimise magnitude and phase at once,
 * which no vertex can do; and the best-gain search compares against a
 * boundary extreme over the box's phase span instead of the allowed set at
 * the phase of the point it certifies, so it can return a gain that
 * violates the boundaries. Algorithm MC of the thesis (algorithm_mc_thesis)
 * reproduces the chapters with those readings repaired one by one, as its
 * own header documents; MC2 is where the formulation is rebuilt instead of
 * patched.
 *
 * The corrections MC2 carries, each proved in the formalisation of
 * documentos/Tesis (03-validez-de-las-mejoras, sec. 1bis):
 *
 * - T1, the correct-vertex theorem: the four cuts are valid if and only if
 *   the other parameters sit at one particular vertex, and the four use only
 *   two vertices, cross-paired - infeasible-magnitude shares its vertex with
 *   feasible-phase, and feasible-magnitude with infeasible-phase. The
 *   pairing is neither {feasible, infeasible} nor {magnitude, phase}, which
 *   is what both publications get wrong.
 * - T2, the slab decomposition: the certified subrange of one parameter is
 *   extracted as a slab, not as the corner of the box, so what is left is
 *   again a box and the 2^n-1 subboxes the article fears never appear.
 * - T3, the exact best gain: with the zeros and poles fixed the phase does
 *   not depend on the gain, so the admissible gains at that point are the
 *   allowed set of one phase column carried to the gain's frame - a finite
 *   union of intervals (RangeUnion), intersected over the design
 *   frequencies. No equation is solved, the empty set propagates on its own
 *   without a sentinel, and the smallest admissible gain can be found in the
 *   lower branch of a closed boundary, which a single-crossing formula never
 *   reaches.
 * - P2, the precondition of the phase equation: with the vertex of T1 and
 *   C_min inside the projected phase span, the angle solved for is below 90
 *   degrees on its own, so the check is an invariant that catches a
 *   phase-branch mistake, not a recovery path.
 *
 * Everything else - the stages of sec. 4.4, the tree bisection of sec. 5.3,
 * the node history, the nominal stability of every certified point and the
 * box-level instability prune - MC2 keeps as MC of the thesis has it, and
 * the two share their search node and stages (common/mc_search_node.h).
 *
 * At this revision MC2 is a faithful copy of MC of the thesis and returns
 * the same result on every fixture, which is the checkpoint the corrections
 * are measured against: each one lands on top, alone, with its own
 * measurement.
 */
namespace qftbx {

class AlgorithmMc2
{
public:

    /**
     * @brief Runtime switches for the thesis strategies, replacing the
     * historical compile-time defines
     * (SACHIN, NAND, the REC_ family, MEJOR_K, BI_ARBOL, ETAPAS).
     *
     * The chapter-6 case studies exercise every improvement alone and in
     * combination, so each one can be disabled independently without
     * rebuilding. All enabled is the thesis MC; everything disabled is the
     * bare branch & bound with area bisection. None of them changes the
     * answer - each only discards boxes it has certified cannot hold a
     * better one - which is what the strategies test asserts. By
     * decision they are not exposed in the interface: a user has no reason
     * to disable a proof.
     */
    struct Strategies {
        bool infeasibleMagnitude = true;  //QSInv, magnitude cuts (NK's QS)
        bool infeasiblePhase = true;      //QSInv, phase cuts (thesis 4.1.2)
        bool feasibleMagnitude = true;    //QSFact, magnitude (thesis 4.1.1)
        bool feasiblePhase = true;        //QSFact, phase
        bool bestGain = true;             //MG (thesis 4.3)
        bool treeBisection = true;        //thesis 4.2.4
        bool stages = true;               //thesis 4.4 (off: always INTERMEDIA)
    };


    void setStrategies(const Strategies & s);

    void setProblem(LtiSystem * plant, LtiSystem * controller, std::vector<double> * omega, const BoundaryData * boundaries,
                   double epsilon);

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

    /// What the run cost, read from the algorithm's own counters.
    LoopShapingStatistics statistics() const;

private:

    //One certainly feasible per-frequency threshold of one parameter
    //(thesis MM/MF): cutting the range at 'threshold' leaves the side
    //named by 'upperSide' feasible for frequency 'freqIndex'.
    struct FeasibleThreshold {
        std::int32_t parameter;   //0 = gain, 1..nz = zero, nz+1.. = pole
        std::size_t freqIndex;
        double threshold;
        bool upperSide;     //true: [threshold, sup] is the feasible part
        double fraction;     //|feasible part| / |range|
    };

    //Detection results of one node, one entry per design frequency
    //(empty for frequencies the node is marked feasible at).
    struct NodeAnalysis {
        std::vector<std::optional<BoxClassification>> classification;
        std::vector<std::optional<NicholsBox>> projection;   //the Nichols box itself
        std::vector<Range> boxMag;     //dB edges of the projected box
        std::vector<Range> boxPhase;   //degree edges
        qftbx::BoxFlag flag = qftbx::feasible;
        std::size_t mainFrequency = 0;    //largest ambiguous projected area
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
    std::unique_ptr<OrderedList> liveList;
    std::vector<std::complex<double>> nominalPlantValues;

    //Prune variable C (thesis 5.4.3): gain and controller of the best
    //certified solution found by MG.
    double bestCertifiedGain = 0;
    std::unique_ptr<LtiSystem> bestCertifiedController;

    std::unique_ptr<LtiSystem> designedController;

    Strategies strategies;

    double phaseGridStep = 0;
    double phaseSpanWidth = 0;

    bool hasUncertainZeros = false;
    bool hasUncertainPoles = false;

    /// Not owned. Null means this run cannot be cancelled.
    const qftbx::CancellationToken * m_cancellation = nullptr;

    /// Copied whole and read as fields; the defaults are the compiled ones.
    qftbx::Settings m_settings;

};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MC2_H
