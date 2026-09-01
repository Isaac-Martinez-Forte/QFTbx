#ifndef QFTBX_LOOPSHAPING_ALGORITHM_NK_H
#define QFTBX_LOOPSHAPING_ALGORITHM_NK_H

#include <complex>

#include <QVector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/nominal_stability_checker.h"
#include "Modelo/Herramientas/tools.h"

#include "src/core/loopshaping/common_functions.h"

/*
 * Algorithm NK (Paluri/Nataraj and Kubal, "Automatic loop shaping in QFT
 * using hybrid optimization and constraint propagation techniques",
 * Int. J. Robust Nonlinear Control 17:251-264, 2007): the NT branch &
 * bound extended with
 *
 * - Quick Solution (sec. 3.3): before a box enters the live list, the
 *   certainly infeasible subranges of the gain, every zero and every pole
 *   are cut off with the closed-form monotonicity equations (see
 *   quick_solution.h), applied per design frequency with the latest
 *   updated values.
 * - Local optimization (sec. 3.2): a coordinate-pattern search launched
 *   from the leading box when its gain infimum differs by more than 10%
 *   from every previous launch point; a feasible local solution prunes
 *   every node whose gain infimum cannot beat it, clips the gain range of
 *   new boxes, and stands in as the answer if the list ever empties.
 *
 * The feasibility test is completed with the nominal closed-loop
 * stability check (zeros of 1 + L0, demanded by the paper's problem
 * formulation), implemented on the Nichols chart by the Cohen-Chait-Yaniv
 * criterion (NominalStabilityChecker).
 */
class AlgorithmNk
{
public:
    AlgorithmNk();
    ~AlgorithmNk();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, BoundaryData * boundaries,
                   qreal epsilon, qint32 inicializacion);

    bool init_algorithm();

    LtiSystem * getControlador();

private:

    //Starting point of the local search. The historical 'random' option
    //made the result non-deterministic and is gone (decision 2026-09-01);
    //the numeric values are the GUI/orchestrator contract.
    enum StartingPoint {Centre = 0, Extremes = 1};

    inline void check_box_feasibility(LtiSystem * controlador);
    inline LtiSystem * quickSolution(LtiSystem * v, qreal boundMinDb, qreal w,
                                     std::complex<qreal> p0);

    inline void localOptimization(LtiSystem * box);
    inline qreal minimalFeasibleGain(const QVector<qreal> & zeros, const QVector<qreal> & poles,
                                     LtiSystem * box, qint32 & budget);
    inline bool pointIsFeasible(const QVector<qreal> & zeros, const QVector<qreal> & poles,
                                qreal gain);
    inline LtiSystem * pointSystem(const QVector<qreal> & zeros, const QVector<qreal> & poles,
                                   qreal gain);
    inline void startingPoint(LtiSystem * box, QVector<qreal> & zeros,
                              QVector<qreal> & poles, qreal & gain);

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

    LtiSystem * controlador_retorno = nullptr;
    LtiSystem * prototype = nullptr;

    //Local optimization state: the best certified feasible gain (prunes
    //the tree), its controller point, and the previous launch points of
    //the 10% decision rule.
    qreal bestLocalGain = 0;
    LtiSystem * bestLocalController = nullptr;
    QVector<qreal> launchGains;

    //Local search configuration from the GUI: coordinate step and
    //starting-point strategy.
    StartingPoint ini = Centre;

    bool hasUncertainZeros = false;
    bool hasUncertainPoles = false;
};

#endif // QFTBX_LOOPSHAPING_ALGORITHM_NK_H
