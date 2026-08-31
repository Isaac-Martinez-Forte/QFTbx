#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H

#include <complex>

#include <QHash>
#include <QPointF>
#include <QVector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/mc_search_node.h"
#include "src/core/loopshaping/stages.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "Modelo/Herramientas/tools.h"

#include "src/core/loopshaping/common_functions.h"

/*
 * Algorithm MC of the QFTbx thesis (chapters 4 and 5): the NT/NK interval
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
class AlgorithmMcThesis
{
public:

    //Runtime switches for the thesis strategies, replacing the historical
    //compile-time defines (SACHIN/NAND/REC_*/MEJOR_K/BI_ARBOL/ETAPAS):
    //the chapter-6 case studies exercise every improvement alone and in
    //combination, so each one can be disabled independently without
    //rebuilding. All enabled is the thesis MC; everything disabled is the
    //bare branch & bound with area bisection.
    struct Strategies {
        bool infeasibleMagnitude = true;  //QSInv, magnitude cuts (NK's QS)
        bool infeasiblePhase = true;      //QSInv, phase cuts (thesis 4.1.2)
        bool feasibleMagnitude = true;    //QSFact, magnitude (thesis 4.1.1)
        bool feasiblePhase = true;        //QSFact, phase
        bool bestGain = true;             //MG (thesis 4.3)
        bool treeBisection = true;        //thesis 4.2.4
        bool stages = true;               //thesis 4.4 (off: always INTERMEDIA)
    };

    AlgorithmMcThesis();
    ~AlgorithmMcThesis();

    void setStrategies(const Strategies & s);

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega, BoundaryData * boundaries,
                   qreal epsilon);

    bool init_algorithm();

    LtiSystem * getControlador();

private:

    //One certainly feasible per-frequency threshold of one parameter
    //(thesis MM/MF): cutting the range at 'threshold' leaves the side
    //named by 'upperSide' feasible for frequency 'freqIndex'.
    struct FeasibleThreshold {
        qint32 parameter;   //0 = gain, 1..nz = zero, nz+1.. = pole
        qint32 freqIndex;
        qreal threshold;
        bool upperSide;     //true: [threshold, sup] is the feasible part
        qreal fraction;     //|feasible part| / |range|
    };

    //Detection results of one node, one entry per design frequency
    //(nullptr for frequencies the node is marked feasible at).
    struct NodeAnalysis {
        QVector<BoxClassification *> datos;
        QVector<QPointF> boxMag;     //dB edges of the projected box
        QVector<QPointF> boxPhase;   //degree edges
        tools::BoxFlag flag = tools::feasible;
        qint32 mainFrequency = 0;    //largest ambiguous projected area
        bool anyFullPhaseWidth = false;
        ~NodeAnalysis();
    };

    inline bool analyse(McSearchNode * node, NodeAnalysis & out);
    inline void improveNode(McSearchNode * node, NodeAnalysis & analysis,
                            QVector<FeasibleThreshold> & thresholds);
    inline bool bestGainSearch(McSearchNode * node, const NodeAnalysis & analysis);
    inline void feasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                             QVector<FeasibleThreshold> & thresholds, bool & improved);
    inline void infeasibleCuts(McSearchNode * node, const NodeAnalysis & analysis,
                               bool & improved);

    inline FC::McBisectionResult bisect(McSearchNode * node, const NodeAnalysis & analysis,
                                        const QVector<FeasibleThreshold> & thresholds);
    inline FC::McBisectionResult bisectAt(McSearchNode * node, qint32 parameter, qreal point);
    inline qint32 widestByMeasure(McSearchNode * node, qint32 mainFrequency, int measure);

    inline bool boxIsFeasibleAt(LtiSystem * box, qint32 freqIndex);
    inline bool boxIsFeasible(LtiSystem * box);
    inline void insertFeasibleBox(LtiSystem * box, McSearchNode * parent);

    inline qint32 parameterCount(LtiSystem * box) const;
    inline QPointF parameterRange(LtiSystem * box, qint32 parameter) const;
    inline LtiSystem * replaceParameter(LtiSystem * box, qint32 parameter,
                                        QPointF range) const;

    LtiSystem * planta = nullptr;
    LtiSystem * controlador = nullptr;
    QVector<qreal> * omega = nullptr;
    BoundaryData * boundaries = nullptr;
    qreal epsilon = 0;

    NaturalIntervalExtension * conversion = nullptr;
    BoundaryViolationDetector * deteccion = nullptr;
    NominalStabilityChecker * stability = nullptr;
    OrderedList * lista = nullptr;
    QVector<cxsc::complex> * plantas_nominales = nullptr;
    QVector<std::complex<qreal>> * plantas_nominales_std = nullptr;

    //Prune variable C (thesis 5.4.3): gain and controller of the best
    //certified solution found by MG.
    qreal bestCertifiedGain = 0;
    LtiSystem * bestCertifiedController = nullptr;

    LtiSystem * controlador_retorno = nullptr;

    Strategies strategies;

    qreal phaseGridStep = 0;
    qreal phaseSpanWidth = 0;

    bool hasUncertainZeros = false;
    bool hasUncertainPoles = false;
};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MC_THESIS_H
