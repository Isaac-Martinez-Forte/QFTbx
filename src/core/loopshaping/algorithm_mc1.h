#ifndef QFTBX_LOOPSHAPING_ALGORITHM_MC1_H
#define QFTBX_LOOPSHAPING_ALGORITHM_MC1_H

#include <complex>

#include <QPointF>
#include <QVector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "src/core/math/sequence_vectors.h"

#include "src/core/loopshaping/common_functions.h"

/*
 * Algorithm MC (Martinez-Forte and Cervera, "Accelerated quantitative
 * feedback theory interval automatic loop shaping algorithm", Int. J.
 * Robust Nonlinear Control 31, 2021, DOI 10.1002/rnc.5499): the NT/NK
 * interval branch & bound accelerated with the QS2 parameter box
 * reduction, which adds to NK's Quick Solution two information sources:
 *
 * - Stage 2, phase information: when a vertical strip of the box's
 *   Nichols rectangle is certainly forbidden, the phase monotonicity of
 *   every zero/pole term yields closed-form cuts (quick_solution.h), the
 *   horizontal counterpart of the magnitude cuts of stage 1 (= NK's QS).
 * - Stage 3, feasible boxes information: the largest upper subrange
 *   [k_f, sup k] of the gain whose box is certainly feasible at every
 *   design frequency yields a CERTIFIED solution with gain k_f. Its
 *   infimum feeds the prune variable C of the paper's step 3bis: boxes
 *   whose gain infimum cannot improve C are discarded, and every new
 *   box's gain range is capped at C (step 3bis.(b)).
 *
 * QFTbx deviations, documented:
 * - The paper inserts the feasible box z' into the live list as a triple
 *   and splits the remainder u = z - z'; here z' is realised as the
 *   certified controller behind C: the capped boxes ARE u, and the
 *   certified point is returned when the search exhausts the list
 *   without finding anything better. Same prune, same fallback, no
 *   duplicate list entries.
 * - Stage 3 finds k_f by logarithmic bisection over the feasibility test
 *   (the paper leaves the search method unspecified); for closed
 *   boundaries feasibility is not monotonic in k_f, so the bisection may
 *   miss a certificate (never accepts a false one).
 * - The returned point must pass the nominal closed-loop stability
 *   criterion (NominalStabilityChecker), as reviewed for NT/NK.
 */
class AlgorithmMc1
{
public:
    AlgorithmMc1();
    ~AlgorithmMc1();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega, BoundaryData * boundaries,
                   qreal epsilon);

    bool init_algorithm();

    LtiSystem * controllerStructure();

private:

    inline void check_box_feasibility(LtiSystem * controlador);
    inline LtiSystem * quickSolution2(LtiSystem * v, BoxClassification * datos,
                                      const cxsc::cinterval & caja, qreal w,
                                      std::complex<qreal> p0);
    inline void certifiedGainSearch(LtiSystem * box);
    inline bool gainRangeIsFeasible(LtiSystem * box, qreal gainInf, qreal gainSup);

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

    //Prune variable C of the paper's step 3bis: gain and controller of
    //the best certified feasible solution found by QS2 stage 3.
    qreal bestCertifiedGain = 0;
    LtiSystem * bestCertifiedController = nullptr;

    LtiSystem * controlador_retorno = nullptr;

    bool hasUncertainZeros = false;
    bool hasUncertainPoles = false;
};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_MC1_H
