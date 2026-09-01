#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_mc1.h"

#include "src/core/loopshaping/quick_solution.h"

using namespace tools;
using namespace cxsc;
using namespace FC;

namespace quick_solution = qftbx::quick_solution;

namespace {

//Relative tolerance of the stage-3 gain bisection: 1% locates the
//certified gain closely enough for pruning without spending the run time
//it is meant to save.
const qreal kCertifiedGainTolerance = 1.01;

//Step 3bis.(b) of the paper: cap the gain range of a box at the prune
//variable C. Returns the capped replacement (and destroys the original)
//or the box itself when the cap does not apply.
LtiSystem * capGain(LtiSystem * box, qreal cap)
{
    if (!box->gain().isUncertain() ||
            cap <= box->gain().range().min || cap >= box->gain().range().max) {
        return box;
    }

    LtiSystem * capped = box->create(box->name(),
            box->numerator(), box->denominator(),
            Parameter("kv", Range(box->gain().range().min, cap),
                          box->gain().range().min, "kv"),
            box->delay());
    delete box;

    return capped;
}

//Nominal plant phase on the (-2 pi, 0] branch the Nichols boxes use.
qreal nominalPhase(std::complex<qreal> p0)
{
    qreal phi0 = std::arg(p0);

    if (phi0 > 0.0) {
        phi0 -= 2.0 * M_PI;
    }

    return phi0;
}

} // namespace


AlgorithmMc1::AlgorithmMc1()
{
}

AlgorithmMc1::~AlgorithmMc1()
{
}


void AlgorithmMc1::set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega,
                                          BoundaryData * boundaries, qreal epsilon)
{
    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;

    hasUncertainZeros = false;
    for (Parameter & var : this->controlador->numerator()) {
        hasUncertainZeros = hasUncertainZeros || var.isUncertain();
    }

    hasUncertainPoles = false;
    for (Parameter & var : this->controlador->denominator()) {
        hasUncertainPoles = hasUncertainPoles || var.isUncertain();
    }
}


//Main loop: the NT branch & bound (paper, algorithm 5) with QS2 inside
//the feasibility test of every box (steps 1(b-bis) and 4bis) and the
//prune variable C of step 3bis behind bestCertifiedGain.
bool AlgorithmMc1::init_algorithm()
{
    lista = new OrderedList();
    conversion = new NaturalIntervalExtension();
    deteccion = new BoundaryViolationDetector();
    stability = new NominalStabilityChecker(planta, omega);

    bestCertifiedGain = std::numeric_limits<qreal>::infinity();
    bestCertifiedController = nullptr;

    plantas_nominales = new QVector<cxsc::complex>();
    plantas_nominales_std = new QVector<std::complex<qreal>>();

    foreach (qreal o, *omega) {
        std::complex<qreal> c = planta->evaluate(o);
        plantas_nominales_std->append(c);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }

    const auto cleanup = [this]() {
        delete conversion;
        delete lista;
        delete deteccion;
        delete stability;
        delete plantas_nominales;
        delete plantas_nominales_std;
    };

    //Steps 1-2: QS2 and feasibility of the initial box happen inside
    //check_box_feasibility, which inserts it unless certainly infeasible.
    check_box_feasibility(controlador);

    while (true) {

        if (lista->isEmpty()) {
            //The certified solution of QS2 stage 3 stands in when the
            //interval search exhausts the space (the paper keeps its box
            //z' in the list instead; same fallback).
            if (bestCertifiedController != nullptr) {
                controlador_retorno = bestCertifiedController;
                cleanup();
                return true;
            }

            cleanup();
            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        SearchNode * node = static_cast<SearchNode *>(lista->first());
        lista->removeFirst();

        //Step 3bis.(a): a node whose gain infimum cannot improve the
        //certified solution is discarded.
        if (node->system()->gain().range().min >= bestCertifiedGain) {
            delete node;
            continue;
        }

        //Step 3 and Remark 3.1 termination, as reviewed for NT.
        if (node->flag() == feasible || isEpsilonSmall(node->system(), this->epsilon, omega, conversion, plantas_nominales)) {
            if (node->flag() == ambiguous) {
                controlador_retorno = pointFromBox(node->system(), false);

                if (!stability->isNominallyStable(controlador_retorno)) {
                    delete controlador_retorno;
                    delete node;
                    continue;
                }
            } else {
                controlador_retorno = pointFromBox(node->system(), true);
            }

            delete node;
            delete bestCertifiedController;
            cleanup();
            return true;
        }

        //Step 4: bisect along the widest parameter direction.
        struct BisectionResult retur = bisectWidestParameter(node->system());

        delete node;

        //Steps 4bis-6: QS2 + feasibility + insertion.
        check_box_feasibility(retur.v1);
        check_box_feasibility(retur.v2);
    }
}


LtiSystem * AlgorithmMc1::getControlador()
{
    return controlador_retorno;
}


//Feasibility test over every design frequency with the QS2 stages 1-2
//cutting applied per frequency with the latest updated box, and stage 3
//attempted once on the surviving box. Certainly infeasible boxes are
//destroyed; anything else enters the live list.
inline void AlgorithmMc1::check_box_feasibility(LtiSystem * controlador)
{
    BoxClassification * datos;
    BoxFlag flag_final = feasible;

    //Step 3bis.(b): the certified solution caps the useful gain range of
    //every new box.
    controlador = capGain(controlador, bestCertifiedGain);

    qint32 contador = 0;
    cinterval caja;

    foreach (qreal o, *omega) {

        caja = conversion->nicholsBox(controlador, o, plantas_nominales->at(contador));

        datos = deteccion->classifyBox(caja, boundaries, contador);

        if (datos->flag() == infeasible) {
            delete controlador;
            delete datos;
            return;
        }

        if (datos->flag() == ambiguous) {
            flag_final = ambiguous;

            controlador = quickSolution2(controlador, datos, caja, o,
                                         plantas_nominales_std->at(contador));
        }

        delete datos;
        contador++;
    }

    //Nominal closed-loop stability of bounds-feasible boxes, as reviewed
    //for NT/NK.
    if (flag_final == feasible) {
        LtiSystem * point = pointFromBox(controlador, true);
        const bool stable = stability->isNominallyStable(point);
        delete point;

        if (!stable) {
            delete controlador;
            return;
        }
    }

    //QS2 stage 3 on the surviving ambiguous box: a certified feasible
    //gain subrange updates the prune variable C.
    if (flag_final == ambiguous) {
        certifiedGainSearch(controlador);
    }

    lista->insert(new SearchNode(controlador->gain().range().min, controlador, flag_final));
}


//QS2 stages 1 and 2 at one design frequency (paper, algorithm 4): the
//magnitude cuts of NK's Quick Solution when the strip under the boundary
//minimum is certainly forbidden, and the phase cuts when a vertical strip
//is. All cuts run sequentially on the latest updated values.
inline LtiSystem * AlgorithmMc1::quickSolution2(LtiSystem * v, BoxClassification * datos,
                                                             const cxsc::cinterval & caja,
                                                             qreal w, std::complex<qreal> p0)
{
    std::vector<double> zeroInfs, zeroSups, poleInfs, poleSups;
    for (Parameter & var : v->numerator()) {
        zeroInfs.push_back(var.isUncertain() ? var.range().min : var.nominal());
        zeroSups.push_back(var.isUncertain() ? var.range().max : var.nominal());
    }
    for (Parameter & var : v->denominator()) {
        poleInfs.push_back(var.isUncertain() ? var.range().min : var.nominal());
        poleSups.push_back(var.isUncertain() ? var.range().max : var.nominal());
    }

    qreal gainInf = v->gain().range().min;
    const qreal gainSup = v->gain().range().max;

    bool cut = false;

    //-------------------------------------------------- stage 1, magnitude
    //Sound only when the zone under every boundary point is certainly
    //forbidden, certified by the parity classification of the box's lower
    //corner (same gate as NK).
    if (datos->isBottomLeftForbidden()) {

        const qreal boundMin = std::pow(10.0, datos->extremes()[0] / 20.0);

        if (v->gain().isUncertain()) {
            const qreal k = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);

            if (k > gainInf && k < gainSup) {
                gainInf = k;
                cut = true;
            }
        }

        if (hasUncertainZeros) {
            for (qint32 j = 0; j < static_cast<qint32>(zeroInfs.size()); ++j) {
                if (!v->numerator()[j].isUncertain()) {
                    continue;
                }

                const qreal z = quick_solution::zeroCut(boundMin, gainSup, zeroSups,
                                                        poleInfs, j, w, p0);

                if (z > zeroInfs[j] && z < zeroSups[j]) {
                    zeroInfs[j] = z;
                    cut = true;
                }
            }
        }

        if (hasUncertainPoles) {
            for (qint32 j = 0; j < static_cast<qint32>(poleInfs.size()); ++j) {
                if (!v->denominator()[j].isUncertain()) {
                    continue;
                }

                const qreal p = quick_solution::poleCut(boundMin, gainSup, zeroSups,
                                                        poleInfs, j, w, p0);

                if (p > poleInfs[j] && p < poleSups[j]) {
                    poleSups[j] = p;
                    cut = true;
                }
            }
        }
    }

    //------------------------------------------------------ stage 2, phase
    if (hasUncertainZeros || hasUncertainPoles) {

        const qreal phi0 = nominalPhase(p0);
        const qreal salto = (boundaries->phaseRange().y() - boundaries->phaseRange().x()) /
                            (boundaries->phaseCount() - 1);

        const qreal boxPhaseMin = _double(Inf(Im(caja)));
        const qreal boxPhaseMax = _double(Sup(Im(caja)));

        const qreal boundPhaseMin = datos->extremes()[2];
        const qreal boundPhaseMax = datos->extremes()[3];

        //Right strip (phases above the boundary maximum) certainly
        //forbidden, and wider than one grid step of the union.
        if (datos->isTopRightForbidden() && boundPhaseMax < boxPhaseMax - salto) {

            const qreal thetaMax = boundPhaseMax * M_PI / 180.0;

            for (qint32 j = 0; hasUncertainZeros && j < static_cast<qint32>(zeroInfs.size()); ++j) {
                if (!v->numerator()[j].isUncertain()) {
                    continue;
                }

                const qreal z = quick_solution::zeroPhaseCutHigh(thetaMax, phi0, zeroSups,
                                                                 poleInfs, j, w);

                if (z > zeroInfs[j] && z < zeroSups[j]) {
                    zeroInfs[j] = z;
                    cut = true;
                }
            }

            for (qint32 j = 0; hasUncertainPoles && j < static_cast<qint32>(poleInfs.size()); ++j) {
                if (!v->denominator()[j].isUncertain()) {
                    continue;
                }

                const qreal p = quick_solution::polePhaseCutHigh(thetaMax, phi0, zeroSups,
                                                                 poleInfs, j, w);

                if (p > poleInfs[j] && p < poleSups[j]) {
                    poleSups[j] = p;
                    cut = true;
                }
            }
        }

        //Left strip (phases below the boundary minimum) certainly
        //forbidden.
        if (datos->isBottomLeftForbidden() && boundPhaseMin > boxPhaseMin + salto) {

            const qreal thetaMin = boundPhaseMin * M_PI / 180.0;

            for (qint32 j = 0; hasUncertainZeros && j < static_cast<qint32>(zeroInfs.size()); ++j) {
                if (!v->numerator()[j].isUncertain()) {
                    continue;
                }

                const qreal z = quick_solution::zeroPhaseCutLow(thetaMin, phi0, zeroInfs,
                                                                poleSups, j, w);

                if (z > zeroInfs[j] && z < zeroSups[j]) {
                    zeroSups[j] = z;
                    cut = true;
                }
            }

            for (qint32 j = 0; hasUncertainPoles && j < static_cast<qint32>(poleInfs.size()); ++j) {
                if (!v->denominator()[j].isUncertain()) {
                    continue;
                }

                const qreal p = quick_solution::polePhaseCutLow(thetaMin, phi0, zeroInfs,
                                                                poleSups, j, w);

                if (p > poleInfs[j] && p < poleSups[j]) {
                    poleInfs[j] = p;
                    cut = true;
                }
            }
        }
    }

    if (!cut) {
        return v;
    }

    std::vector<Parameter> numerador;
    for (qint32 j = 0; j < static_cast<qint32>(zeroInfs.size()); ++j) {
        Parameter & old = v->numerator()[j];
        numerador.push_back(old.isUncertain()
                ? Parameter(old.name(), Range(zeroInfs[j], zeroSups[j]), zeroInfs[j])
                : Parameter(old.nominal()));
    }

    std::vector<Parameter> denominador;
    for (qint32 j = 0; j < static_cast<qint32>(poleInfs.size()); ++j) {
        Parameter & old = v->denominator()[j];
        denominador.push_back(old.isUncertain()
                ? Parameter(old.name(), Range(poleInfs[j], poleSups[j]), poleInfs[j])
                : Parameter(old.nominal()));
    }

    LtiSystem * nuevo = v->create(v->name(), numerador, denominador,
            v->gain().isUncertain()
                ? Parameter("kv", Range(gainInf, gainSup), gainInf, "kv")
                : Parameter(v->gain().nominal()),
            v->delay());

    delete v;

    return nuevo;
}


//Feasibility of the box with its gain range replaced by
//[gainInf, gainSup] at every design frequency.
inline bool AlgorithmMc1::gainRangeIsFeasible(LtiSystem * box,
                                                           qreal gainInf, qreal gainSup)
{
    LtiSystem * candidate = box->create(box->name(),
            box->numerator(), box->denominator(),
            Parameter("kv", Range(gainInf, gainSup), gainInf, "kv"),
            box->delay());

    bool feasibleEverywhere = true;

    for (qint32 i = 0; i < omega->size() && feasibleEverywhere; ++i) {
        const cinterval caja = conversion->nicholsBox(candidate, omega->at(i),
                                                      plantas_nominales->at(i));
        BoxClassification * datos = deteccion->classifyBox(caja, boundaries, i);
        feasibleEverywhere = (datos->flag() == feasible);
        delete datos;
    }

    delete candidate;

    return feasibleEverywhere;
}


//QS2 stage 3 (paper, algorithm 4): the largest upper gain subrange
//[k_f, sup k] certainly feasible at every design frequency. k_f is
//located by logarithmic bisection over the interval feasibility test and
//the certified point must pass the nominal stability criterion before it
//may prune the search through C.
inline void AlgorithmMc1::certifiedGainSearch(LtiSystem * box)
{
    if (!box->gain().isUncertain()) {
        return;
    }

    const qreal low = box->gain().range().min;
    qreal high = box->gain().range().max;

    if (low <= 0.0 || !gainRangeIsFeasible(box, high, high)) {
        return;
    }

    qreal lo = low;

    if (gainRangeIsFeasible(box, lo, high)) {
        high = lo;
    } else {
        qreal hi = high;

        while (hi / lo > kCertifiedGainTolerance) {
            const qreal mid = std::sqrt(lo * hi);

            if (gainRangeIsFeasible(box, mid, high)) {
                hi = mid;
            } else {
                lo = mid;
            }
        }

        high = hi;
    }

    if (high >= bestCertifiedGain) {
        return;
    }

    //The z' box, and its minimal-gain point as certified solution.
    LtiSystem * zPrime = box->create(box->name(),
            box->numerator(), box->denominator(),
            Parameter("kv", Range(high, box->gain().range().max), high, "kv"),
            box->delay());

    LtiSystem * point = pointFromBox(zPrime, true);

    delete zPrime;

    if (stability->isNominallyStable(point)) {
        bestCertifiedGain = high;
        delete bestCertifiedController;
        bestCertifiedController = point;
    } else {
        delete point;
    }
}
