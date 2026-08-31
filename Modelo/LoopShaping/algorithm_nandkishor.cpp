#include "Modelo/Herramientas/exception.h"
#include "algorithm_nandkishor.h"

#include <QRandomGenerator>

#include "quick_solution.h"

using namespace tools;
using namespace cxsc;
using namespace FC;

namespace quick_solution = qftbx::quick_solution;

Algorithm_nandkishor::Algorithm_nandkishor()
{
}

Algorithm_nandkishor::~Algorithm_nandkishor()
{
}


void Algorithm_nandkishor::set_datos(LtiSystem *planta, LtiSystem *controlador, QVector<qreal> * omega, BoundaryData *boundaries,
                                     qreal epsilon, QVector<QVector<QVector<QPointF> *> *> *reunBounHash,
                                     qreal delta, qint32 inicializacion){

    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
    this->reunBounHash = reunBounHash;
    this->delta = delta;
    this->ini = (tipoInicializacion) inicializacion;

    hasUncertainZeros = false;
    foreach (Parameter * var, *this->controlador->numerator()) {
        hasUncertainZeros = hasUncertainZeros || var->isUncertain();
    }

    hasUncertainPoles = false;
    foreach (Parameter * var, *this->controlador->denominator()) {
        hasUncertainPoles = hasUncertainPoles || var->isUncertain();
    }
}


//Main loop: the NT branch & bound (Tharewal 2005, sec. 3.3.3) with the
//NK additions wired at the paper's steps: local optimization on the
//leading box (steps 5-6 and 18-20) and Quick Solution inside the
//feasibility test of every box (steps 2 and 9).
bool Algorithm_nandkishor::init_algorithm(){

    lista = new ListaOrdenada();
    conversion = new NaturalIntervalExtension();
    deteccion = new DeteccionViolacionBoundaries();
    stability = new NominalStabilityChecker(planta, omega);

    bestLocalGain = std::numeric_limits<qreal>::infinity();
    bestLocalController = nullptr;
    launchGains.clear();

    //Stable prototype for building point controllers: the working box
    //pointer is replaced as Quick Solution rebuilds it.
    prototype = controlador->clone();

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
        delete prototype;
    };

    //Steps 1-3: Quick Solution and feasibility of the initial box happen
    //inside check_box_feasibility, which inserts it unless certainly
    //infeasible.
    check_box_feasibility(controlador);

    while (true) {

        if (lista->esVacia()) {
            //Step 15. A certified feasible local solution stands in as
            //the answer when the interval search exhausts the space (the
            //local point was verified against bounds and stability).
            if (bestLocalController != nullptr) {
                controlador_retorno = bestLocalController;
                cleanup();
                return true;
            }

            cleanup();
            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        Tripleta * tripleta = static_cast<Tripleta *>(lista->recuperarPrimero());
        lista->borrarPrimero();

        //Pruning by the local solution (step 4 of the paper's outline /
        //G-bis of the thesis): a node whose gain infimum cannot improve
        //the certified local solution is discarded.
        if (tripleta->getSistema()->gain()->range().x() >= bestLocalGain) {
            delete tripleta;
            continue;
        }

        //Steps 17-20: local optimization launched from the leading box
        //under the 10% decision rule; a feasible result prunes the list
        //through bestLocalGain.
        localOptimization(tripleta->getSistema());

        if (tripleta->getSistema()->gain()->range().x() >= bestLocalGain) {
            delete tripleta;
            continue;
        }

        //Step 21 and Remark 3.1 termination, as reviewed for NT.
        if (tripleta->getFlags() == feasible || if_less_epsilon(tripleta->getSistema(), this->epsilon, omega, conversion, plantas_nominales)) {
            if (tripleta->getFlags() == ambiguous) {
                controlador_retorno = guardarControlador(tripleta->getSistema(), false);

                if (!stability->isNominallyStable(controlador_retorno)) {
                    delete controlador_retorno;
                    delete tripleta;
                    continue;
                }
            } else {
                controlador_retorno = guardarControlador(tripleta->getSistema(), true);
            }

            delete tripleta;
            delete bestLocalController;
            cleanup();
            return true;
        }

        //Step 8: bisect along the widest parameter direction.
        struct return_bisection retur = split_box_bisection(tripleta->getSistema());

        tripleta->noBorrar2();
        delete tripleta;

        //Steps 9-14: Quick Solution + feasibility + insertion.
        check_box_feasibility(retur.v1);
        check_box_feasibility(retur.v2);
    }
}


LtiSystem * Algorithm_nandkishor::getControlador(){
    return controlador_retorno;
}


//Feasibility test over every design frequency with the NK Quick Solution
//cutting applied per frequency with the latest updated box (paper,
//sec. 3.3: "one always uses the latest updated values"). Certainly
//infeasible boxes are destroyed; anything else enters the live list.
inline void Algorithm_nandkishor::check_box_feasibility(LtiSystem * controlador){

    data_box * datos;
    flags_box flag_final = feasible;

    //Step 20 of the paper: the certified local solution caps the useful
    //gain range of every new box.
    if (bestLocalGain < controlador->gain()->range().y() &&
            bestLocalGain > controlador->gain()->range().x()) {
        LtiSystem * capped = controlador->create(controlador->name(),
                controlador->numerator(), controlador->denominator(),
                new Parameter("kv", QPointF(controlador->gain()->range().x(), bestLocalGain),
                              controlador->gain()->range().x(), "kv"),
                controlador->delay());
        delete controlador->gain();
        controlador->releaseOwnership();
        delete controlador;
        controlador = capped;
    }

    qint32 contador = 0;
    cinterval caja;

    foreach (qreal o, *omega) {

        caja = conversion->nicholsBox(controlador, o, plantas_nominales->at(contador), false);

        datos = deteccion->deteccionViolacionCajaNi(caja, boundaries, contador);

        if (datos->getFlag() == infeasible) {
            delete controlador;
            delete datos;
            return;
        }

        if (datos->getFlag() == ambiguous) {
            flag_final = ambiguous;

            //Quick Solution at this frequency: sound only when the zone
            //under every boundary point is certainly forbidden, certified
            //by the parity classification of the box's lower corner.
            if (!datos->isUniArriba()) {
                controlador = quickSolution(controlador,
                                            datos->getMinimoxMaximos()->at(0),
                                            o, plantas_nominales_std->at(contador));
            }
        }

        delete datos;
        contador++;
    }

    //Nominal closed-loop stability of bounds-feasible boxes (the paper
    //demands the zeros of 1 + L0 in the left half-plane; checked on the
    //Nichols chart).
    if (flag_final == feasible) {
        LtiSystem * point = guardarControlador(controlador, true);
        const bool stable = stability->isNominallyStable(point);
        delete point;

        if (!stable) {
            delete controlador;
            return;
        }
    }

    lista->insertar(new Tripleta(controlador->gain()->range().x(), controlador, flag_final));
}


//Quick Solution (paper sec. 3.3, algorithm QS): cut the certainly
//infeasible subranges of the gain, every zero and every pole with the
//closed-form monotonicity equations, sequentially, using the latest
//updated values. boundMinDb is |B_i|min over the box's phase interval.
inline LtiSystem * Algorithm_nandkishor::quickSolution(LtiSystem * v, qreal boundMinDb,
                                                       qreal w, std::complex<qreal> p0){

    const qreal boundMin = std::pow(10.0, boundMinDb / 20.0);

    QVector<qreal> zeroInfs, zeroSups, poleInfs, poleSups;
    foreach (Parameter * var, *v->numerator()) {
        zeroInfs.append(var->isUncertain() ? var->range().x() : var->nominal());
        zeroSups.append(var->isUncertain() ? var->range().y() : var->nominal());
    }
    foreach (Parameter * var, *v->denominator()) {
        poleInfs.append(var->isUncertain() ? var->range().x() : var->nominal());
        poleSups.append(var->isUncertain() ? var->range().y() : var->nominal());
    }

    qreal gainInf = v->gain()->range().x();
    const qreal gainSup = v->gain()->range().y();

    bool cut = false;

    //Steps (3)-(4): the gain, from below.
    if (v->gain()->isUncertain()) {
        const qreal k = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);

        if (k > gainInf && k < gainSup) {
            gainInf = k;
            cut = true;
        }
    }

    //Steps (5)-(6): every zero, from below.
    if (hasUncertainZeros) {
        for (qint32 j = 0; j < zeroInfs.size(); ++j) {
            if (!v->numerator()->at(j)->isUncertain()) {
                continue;
            }

            const qreal z = quick_solution::zeroCut(boundMin, gainSup, zeroSups,
                                                    poleInfs, j, w, p0);

            if (z > zeroInfs.at(j) && z < zeroSups.at(j)) {
                zeroInfs.replace(j, z);
                cut = true;
            }
        }
    }

    //Steps (7)-(8): every pole, from ABOVE (a larger pole lowers the loop
    //towards the forbidden side; the thesis text says the opposite
    //interval - an erratum, see quick_solution.h).
    if (hasUncertainPoles) {
        for (qint32 j = 0; j < poleInfs.size(); ++j) {
            if (!v->denominator()->at(j)->isUncertain()) {
                continue;
            }

            const qreal p = quick_solution::poleCut(boundMin, gainSup, zeroSups,
                                                    poleInfs, j, w, p0);

            if (p > poleInfs.at(j) && p < poleSups.at(j)) {
                poleSups.replace(j, p);
                cut = true;
            }
        }
    }

    if (!cut) {
        return v;
    }

    auto * numerador = new QVector<Parameter*>();
    for (qint32 j = 0; j < zeroInfs.size(); ++j) {
        Parameter * old = v->numerator()->at(j);
        numerador->append(old->isUncertain()
                ? new Parameter(old->name(), QPointF(zeroInfs.at(j), zeroSups.at(j)), zeroInfs.at(j))
                : new Parameter(old->nominal()));
    }

    auto * denominador = new QVector<Parameter*>();
    for (qint32 j = 0; j < poleInfs.size(); ++j) {
        Parameter * old = v->denominator()->at(j);
        denominador->append(old->isUncertain()
                ? new Parameter(old->name(), QPointF(poleInfs.at(j), poleSups.at(j)), poleInfs.at(j))
                : new Parameter(old->nominal()));
    }

    LtiSystem * nuevo = v->create(v->name(), numerador, denominador,
            v->gain()->isUncertain()
                ? new Parameter("kv", QPointF(gainInf, gainSup), gainInf, "kv")
                : new Parameter(v->gain()->nominal()),
            v->delay()->clone());

    delete v;

    return nuevo;
}


//Local optimization (paper sec. 3.2; the paper only says "call any
//nonlinear constrained local optimization routine", so the routine is
//ours): a lean two-level pattern search. The objective is the gain alone,
//so the inner level finds the minimal feasible gain for fixed zeros/poles
//by logarithmic bisection (the predicate is the point bounds test; local
//crossing only, as a local method promises), and the outer level moves
//the zeros/poles with a Hooke-Jeeves style coordinate pattern in LOG
//space with an adaptive, coarsening step. A hard evaluation budget keeps
//the search cheaper than the pruning it buys, and the candidate must pass
//the nominal stability criterion once, at the end, before it may prune
//the global search. Launched under the paper's 10% decision rule. (The
//GUI 'delta' step no longer applies: the step adapts; the parameter is
//kept for compatibility until the phase-8 GUI pass.)

namespace {
const qint32 kLocalSearchBudget = 400;
const qreal kGainTolerance = 1.01;      //1% is plenty for a pruning bound
}

inline qreal Algorithm_nandkishor::minimalFeasibleGain(const QVector<qreal> & zeros,
                                                       const QVector<qreal> & poles,
                                                       LtiSystem * box, qint32 & budget){

    qreal high = box->gain()->range().y();
    qreal low = box->gain()->range().x();

    budget--;
    if (!pointIsFeasible(zeros, poles, high)) {
        return std::numeric_limits<qreal>::infinity();
    }

    budget--;
    if (pointIsFeasible(zeros, poles, low)) {
        return low;
    }

    while (high / low > kGainTolerance && budget > 0) {
        const qreal mid = std::sqrt(low * high);

        budget--;
        if (pointIsFeasible(zeros, poles, mid)) {
            high = mid;
        } else {
            low = mid;
        }
    }

    return high;
}

inline void Algorithm_nandkishor::localOptimization(LtiSystem * box){

    const qreal launch = box->gain()->range().x();

    foreach (qreal previous, launchGains) {
        if (std::abs(launch - previous) <= 0.1 * std::max<qreal>(1.0, std::abs(previous))) {
            return;
        }
    }

    launchGains.append(launch);

    QVector<qreal> zeros, poles;
    qreal gain;
    startingPoint(box, zeros, poles, gain);

    qint32 budget = kLocalSearchBudget;

    qreal bestGain = minimalFeasibleGain(zeros, poles, box, budget);
    QVector<qreal> bestZeros = zeros;
    QVector<qreal> bestPoles = poles;

    //Coordinate pattern over zeros/poles in log space, coarse to fine.
    const auto logRange = [](Parameter * var) {
        return std::log10(var->range().y()) - std::log10(std::max<qreal>(var->range().x(), 1e-12));
    };

    const auto tryMove = [&](bool isPole, qint32 j, qreal stepDecades) -> bool {
        Parameter * var = isPole ? box->denominator()->at(j) : box->numerator()->at(j);
        QVector<qreal> & values = isPole ? bestPoles : bestZeros;

        for (qreal direction : {stepDecades, -stepDecades}) {
            const qreal candidate = values.at(j) * std::pow(10.0, direction);

            if (candidate <= var->range().x() || candidate >= var->range().y()) {
                continue;
            }

            QVector<qreal> trial = values;
            trial.replace(j, candidate);

            const qreal k = isPole ? minimalFeasibleGain(bestZeros, trial, box, budget)
                                   : minimalFeasibleGain(trial, bestPoles, box, budget);

            if (k < bestGain / kGainTolerance) {
                values = trial;
                bestGain = k;
                return true;
            }
        }

        return false;
    };

    for (qreal divisor : {4.0, 8.0, 16.0}) {
        bool improved = true;

        while (improved && budget > 0) {
            improved = false;

            for (qint32 j = 0; j < bestZeros.size() && budget > 0; ++j) {
                if (box->numerator()->at(j)->isUncertain()) {
                    improved = tryMove(false, j, logRange(box->numerator()->at(j)) / divisor) || improved;
                }
            }

            for (qint32 j = 0; j < bestPoles.size() && budget > 0; ++j) {
                if (box->denominator()->at(j)->isUncertain()) {
                    improved = tryMove(true, j, logRange(box->denominator()->at(j)) / divisor) || improved;
                }
            }
        }
    }

    if (bestGain < bestLocalGain) {
        LtiSystem * candidate = pointSystem(bestZeros, bestPoles, bestGain);

        if (stability->isNominallyStable(candidate)) {
            bestLocalGain = bestGain;
            delete bestLocalController;
            bestLocalController = candidate;
        } else {
            delete candidate;
        }
    }
}


inline LtiSystem * Algorithm_nandkishor::pointSystem(const QVector<qreal> & zeros,
                                                     const QVector<qreal> & poles, qreal gain){
    auto * numerador = new QVector<Parameter*>();
    foreach (qreal z, zeros) {
        numerador->append(new Parameter(z));
    }
    auto * denominador = new QVector<Parameter*>();
    foreach (qreal p, poles) {
        denominador->append(new Parameter(p));
    }
    return prototype->create(prototype->name(), numerador, denominador,
                             new Parameter(gain), new Parameter(qreal(0)));
}


//Point feasibility against the bounds at every design frequency, with the
//same projection + detection the interval test uses (the historical local
//search passed the GAIN as the frequency index of the detection).
inline bool Algorithm_nandkishor::pointIsFeasible(const QVector<qreal> & zeros,
                                                  const QVector<qreal> & poles, qreal gain){

    if (gain <= 0.0 || std::isinf(gain)) {
        return false;
    }

    LtiSystem * point = pointSystem(zeros, poles, gain);

    for (qint32 i = 0; i < omega->size(); ++i) {
        const cinterval caja = conversion->nicholsBox(point, omega->at(i),
                                                      plantas_nominales->at(i), false);
        data_box * datos = deteccion->deteccionViolacionCajaNi(caja, boundaries, i);
        const flags_box flag = datos->getFlag();
        delete datos;

        if (flag != feasible) {
            delete point;
            return false;
        }
    }

    delete point;
    return true;
}


//Starting point of the local search, per the GUI choice: box centre,
//random point, or the |L0|-maximal corner.
inline void Algorithm_nandkishor::startingPoint(LtiSystem * box, QVector<qreal> & zeros,
                                                QVector<qreal> & poles, qreal & gain){

    const auto pick = [this](Parameter * var, bool isPole) -> qreal {
        if (!var->isUncertain()) {
            return var->nominal();
        }
        const QPointF r = var->range();
        switch (ini) {
        case centro:
            return (r.x() + r.y()) / 2.0;
        case aleatorio:
            return r.x() + QRandomGenerator::global()->generateDouble() * (r.y() - r.x());
        case extremos:
        default:
            return isPole ? r.y() : r.x();
        }
    };

    zeros.clear();
    poles.clear();

    foreach (Parameter * var, *box->numerator()) {
        zeros.append(pick(var, false));
    }
    foreach (Parameter * var, *box->denominator()) {
        poles.append(pick(var, true));
    }

    Parameter * k = box->gain();
    if (!k->isUncertain()) {
        gain = k->nominal();
    } else if (ini == centro) {
        gain = (k->range().x() + k->range().y()) / 2.0;
    } else if (ini == aleatorio) {
        gain = k->range().x() + QRandomGenerator::global()->generateDouble() *
                (k->range().y() - k->range().x());
    } else {
        gain = k->range().y();
    }
}
