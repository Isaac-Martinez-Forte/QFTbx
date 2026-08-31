#ifndef ALGORITHM_RAMBABU_H
#define ALGORITHM_RAMBABU_H

#include <complex>

#include <QString>
#include <QVector>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "NaturalIntervalExtension/natural_interval_extension.h"
#include "EstructuraDatos/listaordenada.h"
#include "EstructuraDatos/tripleta.h"
#include "EstructuraDatos/arbol_exp.h"
#include "nominal_stability_checker.h"
#include "Modelo/Herramientas/tools.h"

#include "funcionescomunes.h"

/*
 * Algorithm MR (Rambabu Kalla and Nataraj, "Synthesis of fractional-order
 * QFT controllers using interval constraint satisfaction technique",
 * FDA 2010): QFT synthesis as an interval constraint satisfaction
 * problem. The specifications translate into quadratic inequalities in
 * the controller magnitude g and phase phi, one per plant template
 * representative p e^{j theta} and design frequency (and one per ORDERED
 * representative pair for the tracking spread), eqs. (9)-(11) of the
 * paper. The ICSP is solved by branch & prune: every box is narrowed by
 * the HC4 hull-consistency filter (EstructuraDatos/arbol_exp) over the
 * constraint set to a fixpoint; an emptied domain discards the box, and
 * the search bisects the widest variable otherwise. Unlike NT/NK, no
 * Nichols boundaries are needed: the constraints come straight from the
 * specifications and the templates.
 *
 * QFTbx deviations, documented:
 * - The live list is ordered by ascending gain infimum and the search
 *   stops at the first certainly feasible box (every constraint's
 *   interval evaluation non-negative) or at the epsilon-small leading box
 *   (the projection criterion shared by the other algorithms; the paper
 *   collects ALL solution boxes of parameter width epsilon and sorts them
 *   afterwards).
 * - The template contour is subsampled to a handful of representatives
 *   per frequency (the paper uses 9 plants; the full contour would square
 *   into the tracking pairs).
 * - The paper's eq. (9) (plain robust stability) is a square lower-bounded
 *   by zero: it never contracts anything and is omitted.
 * - The returned point must pass the nominal closed-loop stability
 *   criterion (NominalStabilityChecker).
 */
class Algorithm_rambabu
{
public:
    Algorithm_rambabu();
    ~Algorithm_rambabu();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega, BoundaryData * boundaries,
                   qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash, bool depuracion,
                   QVector<QVector<std::complex<qreal>> *> * temp, QVector<tools::dBND *> * espe);

    bool init_algorithm();

    LtiSystem * getControlador();

private:

    struct BoxDomains {
        QMap<std::string, cxsc::interval> values;
    };

    inline void buildControllerExpressions();
    inline void buildConstraints();
    inline void classifyAndInsert(LtiSystem * box);
    inline bool narrowToFixpoint(QMap<std::string, cxsc::interval> & domains);
    inline bool certainlyFeasible(QMap<std::string, cxsc::interval> & domains);
    inline void loadDomains(LtiSystem * box, QMap<std::string, cxsc::interval> & domains);
    inline LtiSystem * boxFromDomains(LtiSystem * box,
                                      const QMap<std::string, cxsc::interval> & domains);

    LtiSystem * planta = nullptr;
    LtiSystem * controlador = nullptr;
    QVector<qreal> * omega = nullptr;
    BoundaryData * boundaries = nullptr;
    qreal epsilon = 0;
    QVector<QVector<QVector<QPointF> *> *> * reunBounHash = nullptr;
    QVector<QVector<std::complex<qreal>> *> * temp = nullptr;
    QVector<tools::dBND *> * espe = nullptr;

    NaturalIntervalExtension * conversion = nullptr;
    NominalStabilityChecker * stability = nullptr;
    ListaOrdenada * lista = nullptr;
    QVector<cxsc::complex> * plantas_nominales = nullptr;

    //Controller magnitude/phase expression strings, one per design
    //frequency, and the parsed constraint trees (built once; each box
    //only reloads the variable domains).
    QVector<QString> magnitudeExpressions;
    QVector<QString> phaseExpressions;
    QVector<alg::exp_tree *> constraints;
    //The source text of each constraint, for diagnostics.
    QVector<QString> constraintTexts;

    LtiSystem * controlador_retorno = nullptr;

    bool depuracion = false;
};

#endif // ALGORITHM_RAMBABU_H
