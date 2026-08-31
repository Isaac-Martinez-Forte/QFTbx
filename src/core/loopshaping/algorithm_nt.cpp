#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_nt.h"
#include <iostream>

using namespace tools;
using namespace cxsc;
using namespace FC;

/*
 * Algorithm NT (Nataraj-Tharewal): interval branch & bound QFT loop
 * shaping, faithful to Tharewal 2005 ("Automated Synthesis of QFT
 * Controllers and Prefilters using Interval Global Optimization
 * Techniques", IIT Bombay):
 *
 * - chapter 3 (sec. 3.3.3): the branch & bound over the controller
 *   parameter box with the live-node list NL ordered by ascending
 *   inf(k), so the first solution is the global optimum;
 * - chapter 5 (sec. 5.2.1): the constraint-propagation acceleration on
 *   the gain, using the monotonicity of |L0| with respect to k and the
 *   extreme boundary magnitudes B_min / B_max over the box's phase
 *   interval: the certainly infeasible gain subrange is cut off (C_g-)
 *   and the certainly feasible one is split into its own NL triple
 *   (C_g+).
 *
 * Termination follows ch. 3 (p. 29 and Remark 3.1): a feasible leading
 * box, or a leading box whose Nichols projection is smaller than epsilon
 * at every design frequency. In the second case, when the box is still
 * ambiguous, the returned point is the corner of the box that the
 * monotonicity of the projection makes feasible (the anti-blocking rule
 * of the QFTbx thesis, sec. 3.1).
 *
 * The feasibility test is completed with the nominal closed-loop
 * stability check of sec. 3.3.5, implemented on the Nichols chart by the
 * Cohen-Chait-Yaniv criterion (NominalStabilityChecker): the QFT bounds
 * alone do not exclude loops that encircle the critical point. The
 * historical code approximated this with a hard-coded ordering penalty
 * at 2 rad/s, retired by this review.
 */

AlgorithmNt::AlgorithmNt() {

}

AlgorithmNt::~AlgorithmNt() {

}

void AlgorithmNt::set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, BoundaryData * boundaries,
                                 qreal epsilon) {


    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;

    this->metaDatosArriba = boundaries->upperFlags();
    this->metaDatosAbierto = boundaries->openFlags();

    this->tamFas = boundaries->phaseCount() - 1;
}


//Main loop: Tharewal 2005, sec. 3.3.3 (steps 1-7).

bool AlgorithmNt::init_algorithm() {

    lista = new OrderedList();

    conversion = new NaturalIntervalExtension();
    deteccion = new BoundaryViolationDetector();
    stability = new NominalStabilityChecker(planta, omega);

    plantas_nominales = new QVector <cxsc::complex> ();

    foreach (qreal o, *omega) {
        std::complex <qreal> c = planta->evaluate(o);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }

    //Step 1: feasibility of the initial search box (inserts it into NL
    //unless certainly infeasible).
    check_box_feasibility(controlador);


    while (true) {

        //Steps 2/6c: an empty list proves there is no feasible solution.
        if (lista->isEmpty()) {
            delete conversion;
            delete lista;
            delete deteccion;
            delete stability;

            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        SearchNode * node = static_cast<SearchNode *>(lista->first());
        lista->removeFirst();


        //Step 3, termination: a feasible leading box (ch. 3, p. 29; its
        //lower gain corner realises the optimum), or a leading box below
        //the epsilon accuracy at every frequency (Remark 3.1; if still
        //ambiguous, the feasible corner is extracted).
        if (node->flag() == feasible || isEpsilonSmall(node->system(), this->epsilon, omega, conversion, plantas_nominales)) {
            if (node->flag() == ambiguous) {
                controlador_retorno = pointFromBox(node->system(), false);

                //The anti-blocking corner is a fresh point: it must pass
                //the nominal stability criterion too. If it does not,
                //this node yields no solution and the search continues.
                if (!stability->isNominallyStable(controlador_retorno)) {
                    delete controlador_retorno;
                    delete node;
                    continue;
                }
            } else {
                //The lower corner of a feasible box was already certified
                //when the box entered the list.
                controlador_retorno = pointFromBox(node->system(), true);
            }

            delete conversion;
            delete lista;
            delete node;
            delete deteccion;
            delete stability;

            return true;
        }

        //Step 4: bisect along the widest parameter direction.
        struct BisectionResult retur = bisectWidestParameter(node->system());

        delete node;

        //Steps 5-6: classify the subboxes and insert them in NL.
        check_box_feasibility(retur.v1);
        check_box_feasibility(retur.v2);
    }
}


LtiSystem * AlgorithmNt::getControlador() {
    return controlador_retorno;
}


//Feasibility test of one box over every design frequency (Tharewal 2005,
//sec. 3.3.4-3.3.5) plus the ch. 5 gain acceleration. Certainly infeasible
//boxes are destroyed; anything else is inserted into NL ordered by
//inf(k). When the certainly feasible gain subrange [feasibleFrom, sup(k)]
//can be split off (C_g+), it is re-certified by this same test and
//enters NL as its own triple.

inline void AlgorithmNt::check_box_feasibility(LtiSystem * controlador) {

    BoxClassification * datos;

    BoxFlag flag_final = feasible;

    qint32 contador = 0;
    cinterval caja;

    //C_g+ : the certainly feasible gain subrange must satisfy EVERY
    //frequency (intersection), so the candidate is the maximum of the
    //per-frequency lower limits and fails if any ambiguous frequency
    //cannot certify one.
    qreal feasibleFrom = 0;
    bool feasibleCertified = true;

    foreach(qreal o, *omega) {

        caja = conversion->nicholsBox(controlador, o, plantas_nominales->at(contador));

        datos = deteccion->classifyBox(caja, boundaries, contador);

        if (datos->flag() == infeasible) {
            delete controlador;
            delete datos;

            return;
        }

        if (datos->flag() == ambiguous) {
            flag_final = ambiguous;

            const qreal minimoBoundarie = datos->extremes()[0];
            const qreal maximoBoundarie = datos->extremes()[1];

            //C_g- : cut the certainly infeasible low-gain subrange.
            controlador = acelerated(controlador, minimoBoundarie, o, contador, !datos->isBottomLeftForbidden());

            //C_g+ : candidate lower limit of the certainly feasible
            //high-gain subrange at this frequency.
            if (feasibleCertified) {
                qreal from;
                if (feasibleGainFrom(controlador, maximoBoundarie, caja, o, contador, from)) {
                    feasibleFrom = std::max(feasibleFrom, from);
                } else {
                    feasibleCertified = false;
                }
            }
        }

        delete datos;

        contador++;
    }

    //C_g+ split (Tharewal 2005, sec. 5.2.1-5.2.2): the candidate feasible
    //part becomes its own box and is re-certified by this same test, so
    //the split never depends on the heuristic gate for correctness. The
    //margins skip degenerate slivers that would only bloat the list.
    const qreal kInf = controlador->gain()->range().x();
    const qreal kSup = controlador->gain()->range().y();

    //Nominal closed-loop stability of bounds-feasible boxes (Tharewal
    //2005, sec. 3.3.5, by the Nichols-chart Nyquist criterion): satisfied
    //stability bounds plus one nominally stable point make the whole box
    //robustly stable; an unstable point discards it entirely.
    if (flag_final == feasible) {
        LtiSystem * point = pointFromBox(controlador, true);
        const bool stable = stability->isNominallyStable(point);
        delete point;

        if (!stable) {
            delete controlador;
            return;
        }
    }

    if (flag_final == ambiguous && feasibleCertified &&
            feasibleFrom > kInf * 1.01 && feasibleFrom < kSup * 0.99) {

        //Deep copy for the feasible part, with its own gain interval.
        LtiSystem * base = controlador->clone();
        LtiSystem * feasiblePart = base->create(base->name(), base->numerator(),
                base->denominator(),
                new Parameter("kv", QPointF(feasibleFrom, kSup), feasibleFrom, "kv"),
                base->delay());
        delete base->gain();
        base->releaseOwnership();
        delete base;

        check_box_feasibility(feasiblePart);

        //The current box keeps the remaining ambiguous gain subrange.
        LtiSystem * ambiguousPart = controlador->create(controlador->name(),
                controlador->numerator(), controlador->denominator(),
                new Parameter("kv", QPointF(kInf, feasibleFrom), kInf, "kv"),
                controlador->delay());

        delete controlador->gain();
        controlador->releaseOwnership();
        delete controlador;

        controlador = ambiguousPart;
    }

    lista->insert(new SearchNode(controlador->gain()->range().x(), controlador, flag_final));

}


//Geometric contractor C_g- (Tharewal 2005, ch. 5, Algorithm C_g-): using
//the monotonicity of |L0| w.r.t. the gain, remove the gain subrange
//[inf(k), k_B] whose boxes lie entirely below B_min, the minimum boundary
//magnitude over the box's phase interval. The cut only applies when the
//below-everything zone is certainly forbidden, certified by the parity
//classification of the box's lower corner (arriba == false).

inline LtiSystem * AlgorithmNt::acelerated(LtiSystem *v, qreal minimo_boundarie, qreal o, qint32 contador, bool arriba) {

    if (!arriba){

        Parameter * min_k_lineal = new Parameter(v->gain()->range().x());
        qreal min_k_db = 20 * log10(min_k_lineal->range().x());

        LtiSystem * G_k_min = v->create(v->name(), v->numerator(), v->denominator(),
                                      min_k_lineal, v->delay());


        qreal mag_min_db = _double(SupRe(conversion->nicholsBox(G_k_min, o, plantas_nominales->at(contador))));

        delete min_k_lineal;
        G_k_min->releaseOwnership();
        delete G_k_min;


        if (mag_min_db < minimo_boundarie) {

            //k_B = inf(k) + (B_min - sup|L0(inf(k))|), in dB.
            qreal Kb_db = min_k_db + (minimo_boundarie - mag_min_db);

            qreal Kb_lineal = pow(10, Kb_db / 20);

            LtiSystem * nuevo_sistema = v->create(v->name(), v->numerator(), v->denominator(),
                                                new Parameter("kv", QPointF(Kb_lineal, v->gain()->range().y()), Kb_lineal, "kv"), v->delay());

            delete v->gain();
            v->releaseOwnership();
            delete v;

            v = nuevo_sistema;
        }
    }

    return v;
}


//Geometric contractor C_g+ (Tharewal 2005, ch. 5): lower limit of the
//gain subrange whose boxes lie entirely above B_max, the maximum boundary
//magnitude over the box's phase interval. Returns false when no part of
//the gain range can be certified feasible at this frequency. The zone
//above every boundary point must be an allowed zone, checked by the
//parity classification of a probe point just above B_max at the centre
//of the box's phase interval (a heuristic gate: the caller re-certifies
//the split box with the full feasibility test).

inline bool AlgorithmNt::feasibleGainFrom(LtiSystem * v, qreal maximo_boundarie,
                                               cinterval caja, qreal o, qint32 contador, qreal & from) {

    const qreal centroFase = (_double(InfIm(caja)) + _double(SupIm(caja))) / 2.0;

    if (deteccion->classifyPoint(QPointF(centroFase, maximo_boundarie + 1.0),
                                   boundaries, contador) != feasible) {
        return false;
    }

    Parameter * max_k_lineal = new Parameter(v->gain()->range().y());
    qreal max_k_db = 20 * log10(max_k_lineal->range().x());

    LtiSystem * G_k_max = v->create(v->name(), v->numerator(), v->denominator(),
                                  max_k_lineal, v->delay());

    qreal mag_max_db = _double(InfRe(conversion->nicholsBox(G_k_max, o, plantas_nominales->at(contador))));

    delete max_k_lineal;
    G_k_max->releaseOwnership();
    delete G_k_max;

    if (mag_max_db <= maximo_boundarie) {
        return false;
    }

    //k_F = sup(k) - (inf|L0(sup(k))| - B_max), in dB.
    const qreal Kf_db = max_k_db - (mag_max_db - maximo_boundarie);

    from = pow(10, Kf_db / 20);

    return true;
}
