#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H

#include "src/core/project/settings.h"
#include "src/core/pipeline/cancellation.h"
#include <cstdint>
#include <complex>
#include <optional>

#include <vector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/mc_search_node.h"
#include "src/core/loopshaping/stages.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common_functions.h"

/**
 * @brief Algorithm MC of the QFTbx thesis: the NT/NK branch & bound with
 * every strategy of chapter 4.
 *
 * Thesis chapters 4 and 5: the NT/NK interval
 * branch & bound extended with every strategy of chapter 4, assembled as
 * the pseudocode of chapter 5 prescribes:
 *
 * - QSInv (thesis 5.1.1): the certainly infeasible subranges of every
 *   controller parameter are cut away with the closed-form magnitude
 *   equations of NK's Quick Solution and the phase equations of thesis
 *   sec. 4.1.2 (quick_solution.h), on whichever sides of the projected
 *   box the boundary certifies as forbidden.
 * - QSFact (thesis 5.1.2): the certainly FEASIBLE subranges of every
 *   parameter, per design frequency, with the same equations evaluated at
 *   the opposite corner and the opposite boundary extreme (B_max/C_min/
 *   C_max). The subrange feasible at EVERY frequency is split off the box
 *   and enters the live list as a feasible node (UM/UF); the per-frequency
 *   thresholds (MM/MF) feed the tree bisection.
 * - MG (thesis 5.2): the best-gain search fixes the other parameters at
 *   the corner that maximises the controller magnitude (zeros sup, poles
 *   inf) and intersects the per-frequency feasible gain thresholds,
 *   yielding a point solution with a potentially much lower gain than the
 *   box-certified one. It feeds the prune variable C. QSFact runs only
 *   when MG finds nothing (thesis 5.1.2, they overlap in purpose).
 * - Tree bisection (thesis 5.3): in the intermediate stage the box is
 *   split at the stored per-frequency feasible threshold covering the
 *   largest fraction of its variable's range; the feasible child is
 *   marked feasible for that frequency, and the mark (node history)
 *   skips its feasibility test from then on.
 * - Execution stages (thesis 4.4): INICIAL (area bisection) until no
 *   projected box spans the full phase width; INTERMEDIA (tree bisection)
 *   until a full pass of MG/QSFact/QSInv produces nothing; FINAL (cuts
 *   disabled, bisection by the wider of magnitude/phase).
 *
 * QFTbx deviations and fixes, documented:
 * - MG's certified gain and the feasible nodes must pass the nominal
 *   closed-loop stability criterion (NominalStabilityChecker), as in the
 *   reviewed NT/NK/MR/MC1; MG's candidate is verified against the
 *   feasibility test before it may prune (the closed form alone relies
 *   on strip geometry).
 * - When the live list empties with a certified MG solution standing,
 *   that solution is returned (the thesis pseudocode would report "no
 *   solution" while holding one in C).
 * - The thesis writes |B_min| in the equations of sec. 4.1.1 where its
 *   text prescribes B_max, states in MG (algorithm 5.3) k_f as the
 *   subs(z,...) box where only the corner point is certified, and the
 *   QSInv comment says the fixed corner "maximises" the phase
 *   contribution where the assignments minimise it: the implementations
 *   here follow the sound readings (errata candidates).
 */
namespace qftbx {

class AlgorithmMcThesis
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
     * better one - which is what mc_thesis_strategies_test asserts. By
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
        std::vector<std::optional<cxsc::cinterval>> projection;   //the Nichols box itself
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
    std::vector<cxsc::complex> nominalPlantValues;
    std::vector<std::complex<double>> nominalPlantValuesStd;

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

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H
