#include "Modelo/Herramientas/exception.h"
#include "algorithm_mc_thesis.h"

#include "quick_solution.h"

using namespace tools;
using namespace cxsc;
using namespace FC;

namespace quick_solution = qftbx::quick_solution;

namespace {

//Prune step of thesis 5.4.3: cap the gain range of a box at the prune
//variable C. Returns the capped replacement (destroying the original) or
//the box itself when the cap does not apply.
LtiSystem * capGain(LtiSystem * box, qreal cap)
{
    if (!box->gain()->isUncertain() ||
            cap <= box->gain()->range().x() || cap >= box->gain()->range().y()) {
        return box;
    }

    LtiSystem * capped = box->create(box->name(),
            box->numerator(), box->denominator(),
            new Parameter("kv", QPointF(box->gain()->range().x(), cap),
                          box->gain()->range().x(), "kv"),
            box->delay());
    delete box->gain();
    box->releaseOwnership();
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

//Corner value vectors of a box (uncertain parameters at the requested
//extreme, fixed ones at their nominal).
void cornerVectors(LtiSystem * box, bool zerosAtSup, bool polesAtSup,
                   QVector<qreal> & zeros, QVector<qreal> & poles)
{
    zeros.clear();
    poles.clear();

    foreach (Parameter * var, *box->numerator()) {
        zeros.append(!var->isUncertain() ? var->nominal()
                     : (zerosAtSup ? var->range().y() : var->range().x()));
    }
    foreach (Parameter * var, *box->denominator()) {
        poles.append(!var->isUncertain() ? var->nominal()
                     : (polesAtSup ? var->range().y() : var->range().x()));
    }
}

//Deep destruction of a node: Tripleta keeps the system alive by default
//(the legacy split shares internals); here every child is a deep copy,
//so the popped node's system dies with it.
void destroyNode(Tripleta2 * node)
{
    node->releaseOwnership();
    node->noBorrar2();
    delete node;
}

} // namespace


AlgorithmMcThesis::NodeAnalysis::~NodeAnalysis()
{
    foreach (data_box * d, datos) {
        delete d;
    }
}


AlgorithmMcThesis::AlgorithmMcThesis()
{
}

AlgorithmMcThesis::~AlgorithmMcThesis()
{
}


void AlgorithmMcThesis::setStrategies(const Strategies & s)
{
    strategies = s;
}


void AlgorithmMcThesis::set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> * omega,
                                  BoundaryData * boundaries, qreal epsilon)
{
    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;

    phaseSpanWidth = boundaries->phaseRange().y() - boundaries->phaseRange().x();
    phaseGridStep = phaseSpanWidth / (boundaries->phaseCount() - 1);

    hasUncertainZeros = false;
    foreach (Parameter * var, *this->controlador->numerator()) {
        hasUncertainZeros = hasUncertainZeros || var->isUncertain();
    }

    hasUncertainPoles = false;
    foreach (Parameter * var, *this->controlador->denominator()) {
        hasUncertainPoles = hasUncertainPoles || var->isUncertain();
    }
}


//--------------------------------------------------------- parameter access
//Uniform view of the controller parameters: 0 is the gain, then the
//zeros, then the poles (the thesis' x vector).

inline qint32 AlgorithmMcThesis::parameterCount(LtiSystem * box) const
{
    return 1 + box->numerator()->size() + box->denominator()->size();
}

inline QPointF AlgorithmMcThesis::parameterRange(LtiSystem * box, qint32 parameter) const
{
    Parameter * var;

    if (parameter == 0) {
        var = box->gain();
    } else if (parameter <= box->numerator()->size()) {
        var = box->numerator()->at(parameter - 1);
    } else {
        var = box->denominator()->at(parameter - 1 - box->numerator()->size());
    }

    return var->isUncertain() ? var->range()
                              : QPointF(var->nominal(), var->nominal());
}

//New box with one parameter's range replaced (deep copy, the original is
//left untouched).
inline LtiSystem * AlgorithmMcThesis::replaceParameter(LtiSystem * box, qint32 parameter,
                                                       QPointF range) const
{
    auto * numerador = new QVector<Parameter*>();
    for (qint32 j = 0; j < box->numerator()->size(); ++j) {
        Parameter * old = box->numerator()->at(j);
        numerador->append(parameter == j + 1
                ? new Parameter(old->name(), range, range.x())
                : old->clone());
    }

    auto * denominador = new QVector<Parameter*>();
    for (qint32 j = 0; j < box->denominator()->size(); ++j) {
        Parameter * old = box->denominator()->at(j);
        denominador->append(parameter == j + 1 + box->numerator()->size()
                ? new Parameter(old->name(), range, range.x())
                : old->clone());
    }

    Parameter * gain = parameter == 0
            ? new Parameter("kv", range, range.x(), "kv")
            : box->gain()->clone();

    return box->create(box->name(), numerador, denominador, gain,
                       box->delay()->clone());
}


//------------------------------------------------------------ main loop
//Thesis 5.4, algorithm MC: branch & bound over the live list ordered by
//ascending gain infimum, with the prune variable C, the execution stages
//and the cutting/bisection strategies wired per the pseudocode.
bool AlgorithmMcThesis::init_algorithm()
{
    lista = new ListaOrdenada();
    conversion = new NaturalIntervalExtension();
    deteccion = new DeteccionViolacionBoundaries();
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

    //A controller with no uncertain parameter offers nothing to search.
    if (!hasUncertainZeros && !hasUncertainPoles && !controlador->gain()->isUncertain()) {
        controlador_retorno = guardarControlador(controlador, true);
        delete controlador;
        cleanup();
        return false;
    }

    //Step A/B: the initial box enters the list; its feasibility test
    //happens when it is popped (step D).
    Tripleta2 * inicial = new Tripleta2(controlador->gain()->range().x(), controlador, ambiguous);
    inicial->setEtapas(strategies.stages ? Etapas::INICIAL : Etapas::INTERMEDIA);
    inicial->setRecorteActivado(true);
    inicial->setFrecuenciasFeasible(new QHash<qreal, qreal>());
    lista->insertar(inicial);

    while (true) {

        //Step C: pop, prune and cap with C.
        if (lista->esVacia()) {
            //The certified solution of MG stands in when the search
            //exhausts the space (the thesis pseudocode reports "no
            //solution" here even when C holds one; returning it is the
            //sound completion).
            if (bestCertifiedController != nullptr) {
                controlador_retorno = bestCertifiedController;
                cleanup();
                return true;
            }

            cleanup();
            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        Tripleta2 * node = static_cast<Tripleta2 *>(lista->recuperarPrimero());
        lista->borrarPrimero();

        //Strict comparison: a node whose infimum EQUALS C still realises
        //the certified optimum (thesis 5.4.3 prescribes < over <=).
        if (bestCertifiedGain < node->getSistema()->gain()->range().x()) {
            destroyNode(node);
            continue;
        }

        node->setSistema(capGain(node->getSistema(), bestCertifiedGain));

        //A feasible node is a solution: its gain infimum corner realises
        //the optimum of the box (stability was certified at insertion).
        if (node->getFlags() == feasible) {
            controlador_retorno = guardarControlador(node->getSistema(), true);
            destroyNode(node);
            delete bestCertifiedController;
            cleanup();
            return true;
        }

        //Step D: feasibility test of the current box.
        NodeAnalysis analysis;
        if (!analyse(node, analysis)) {
            continue;   //certainly infeasible, destroyed inside
        }

        if (analysis.flag == feasible) {
            controlador_retorno = guardarControlador(node->getSistema(), true);
            destroyNode(node);

            if (!stability->isNominallyStable(controlador_retorno)) {
                delete controlador_retorno;
                continue;
            }

            delete bestCertifiedController;
            cleanup();
            return true;
        }

        //Termination on the epsilon-small leading box (thesis 3.3, the
        //solution function): the returned point is unverified, so it must
        //pass the stability criterion, as reviewed for NT.
        if (if_less_epsilon(node->getSistema(), epsilon, omega, conversion, plantas_nominales)) {
            controlador_retorno = guardarControlador(node->getSistema(), false);
            destroyNode(node);

            if (!stability->isNominallyStable(controlador_retorno)) {
                delete controlador_retorno;
                continue;
            }

            delete bestCertifiedController;
            cleanup();
            return true;
        }

        //Steps E-F: stage bookkeeping, MG, QSFact, QSInv.
        QVector<FeasibleThreshold> thresholds;
        improveNode(node, analysis, thresholds);

        //C may have improved inside F.
        if (bestCertifiedGain < node->getSistema()->gain()->range().x()) {
            destroyNode(node);
            continue;
        }

        //Steps G-H: bisect and insert the children.
        FC::return_bisection2 children = bisect(node, analysis, thresholds);

        for (Tripleta2 * child : {children.t1, children.t2}) {
            if (child == nullptr) {
                continue;
            }

            if (bestCertifiedGain < child->getSistema()->gain()->range().x()) {
                destroyNode(child);
                continue;
            }

            child->setIndex(child->getSistema()->gain()->range().x());
            lista->insertar(child);
        }
    }
}


LtiSystem * AlgorithmMcThesis::getControlador()
{
    return controlador_retorno;
}


//------------------------------------------------------- feasibility test
//Step D: one detection per design frequency (skipping the frequencies
//the node history already certifies as feasible), collecting the data
//the cutting stages and the bisection need. Returns false (destroying
//the node) when some frequency is certainly infeasible.
inline bool AlgorithmMcThesis::analyse(Tripleta2 * node, NodeAnalysis & out)
{
    out.flag = feasible;
    out.mainFrequency = 0;
    out.anyFullPhaseWidth = false;

    qreal largestArea = std::numeric_limits<qreal>::lowest();

    for (qint32 i = 0; i < omega->size(); ++i) {

        if (node->isFrecueciaFeasible(i)) {
            out.datos.append(nullptr);
            out.boxMag.append(QPointF());
            out.boxPhase.append(QPointF());
            continue;
        }

        const cinterval caja = conversion->nicholsBox(node->getSistema(), omega->at(i),
                                                      plantas_nominales->at(i));

        data_box * datos = deteccion->deteccionViolacionCajaNi(caja, boundaries, i);

        if (datos->getFlag() == infeasible) {
            delete datos;
            destroyNode(node);
            return false;
        }

        out.datos.append(datos);
        out.boxMag.append(QPointF(_double(Inf(Re(caja))), _double(Sup(Re(caja)))));
        out.boxPhase.append(QPointF(_double(Inf(Im(caja))), _double(Sup(Im(caja)))));

        const qreal phaseWidth = _double(diam(Im(caja)));

        if (phaseWidth >= phaseSpanWidth - phaseGridStep) {
            out.anyFullPhaseWidth = true;
        }

        if (datos->getFlag() == ambiguous) {
            out.flag = ambiguous;

            const qreal area = _double(diam(Re(caja))) * phaseWidth;
            if (area > largestArea) {
                largestArea = area;
                out.mainFrequency = i;
            }
        }
    }

    return true;
}


//--------------------------------------------------------------- steps E-F
inline void AlgorithmMcThesis::improveNode(Tripleta2 * node, NodeAnalysis & analysis,
                                           QVector<FeasibleThreshold> & thresholds)
{
    //Step E (thesis 4.4): the initial stage ends when no projected box
    //spans the full phase width of the Nichols plane any more.
    if (strategies.stages &&
            node->getEtapas() == Etapas::INICIAL && !analysis.anyFullPhaseWidth) {
        node->setEtapas(Etapas::INTERMEDIA);
    }

    if (!node->isRecorteActivado()) {
        return;
    }

    //Step F: MG first; QSFact only when MG finds nothing (thesis 5.1.2:
    //they overlap in purpose); QSInv always.
    bool improved = false;

    if (strategies.bestGain && bestGainSearch(node, analysis)) {
        improved = true;
    } else if (strategies.feasibleMagnitude || strategies.feasiblePhase) {
        feasibleCuts(node, analysis, thresholds, improved);
    }

    if (strategies.infeasibleMagnitude || strategies.infeasiblePhase) {
        infeasibleCuts(node, analysis, improved);
    }

    //The final stage begins when a full pass yields nothing (thesis 4.4):
    //the cuts are disabled from here on for this node and its children.
    if (strategies.stages && !improved && node->getEtapas() == Etapas::INTERMEDIA) {
        node->setEtapas(Etapas::FINAL);
        node->setRecorteActivado(false);
    }
}


//Feasibility of a box (or point) at one design frequency, and at all of
//them: the defensive verification of everything the closed-form
//certificates produce (MG candidates, UM/UF boxes, tree-bisection
//marks). An equation slip then costs a missed acceleration, never a
//wrong verdict.
inline bool AlgorithmMcThesis::boxIsFeasibleAt(LtiSystem * box, qint32 freqIndex)
{
    const cinterval caja = conversion->nicholsBox(box, omega->at(freqIndex),
                                                  plantas_nominales->at(freqIndex));
    data_box * datos = deteccion->deteccionViolacionCajaNi(caja, boundaries, freqIndex);
    const bool feasibleHere = datos->getFlag() == feasible;
    delete datos;

    return feasibleHere;
}

inline bool AlgorithmMcThesis::boxIsFeasible(LtiSystem * box)
{
    for (qint32 i = 0; i < omega->size(); ++i) {
        if (!boxIsFeasibleAt(box, i)) {
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------- MG
//Best-gain search (thesis 4.3 and 5.2): with the other parameters fixed
//at the corner that maximises the controller magnitude (zeros sup, poles
//inf), each ambiguous frequency yields a closed-form gain threshold on
//its feasible side; their intersection is the best certified gain of the
//box. The candidate is verified against the feasibility test and the
//stability criterion before it may prune through C (the thesis relies on
//the strip geometry alone; the extra checks cost |Omega| detections).
inline bool AlgorithmMcThesis::bestGainSearch(Tripleta2 * node, const NodeAnalysis & analysis)
{
    LtiSystem * box = node->getSistema();

    if (!box->gain()->isUncertain()) {
        return false;
    }

    QVector<qreal> zeroSups, poleInfs;
    cornerVectors(box, true, false, zeroSups, poleInfs);

    const qreal kInf = box->gain()->range().x();
    const qreal kSup = box->gain()->range().y();

    qreal lowNeeded = kInf;    //k must be >= (top-side feasible strips)
    qreal highAllowed = kSup;  //k must be <= (bottom-side feasible strips)

    for (qint32 i = 0; i < omega->size(); ++i) {

        data_box * datos = analysis.datos.value(i);

        if (datos == nullptr || datos->getFlag() != ambiguous) {
            continue;   //the whole box, corner included, is feasible here
        }

        const qreal w = omega->at(i);
        const std::complex<qreal> p0 = plantas_nominales_std->at(i);
        const qreal boundMin = std::pow(10.0, datos->getMinimoxMaximos()->at(0) / 20.0);
        const qreal boundMax = std::pow(10.0, datos->getMinimoxMaximos()->at(1) / 20.0);

        //Preferring the bottom strip serves the objective (it allows the
        //gain infimum); the top strip is the fallback.
        bool constrained = false;

        if (!datos->isUniAbajo()) {   //strip under B_min certainly feasible
            const qreal t = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);

            if (t >= kInf) {
                highAllowed = std::min(highAllowed, t);
                constrained = true;
            }
        }

        if (!constrained && !datos->isUniDerecha()) {   //strip over B_max feasible
            const qreal t = quick_solution::gainCut(boundMax, zeroSups, poleInfs, w, p0);

            if (t <= kSup && t > 0.0) {
                lowNeeded = std::max(lowNeeded, t);
                constrained = true;
            }
        }

        if (!constrained) {
            return false;   //this frequency cannot be certified at the corner
        }
    }

    if (lowNeeded > highAllowed || lowNeeded >= bestCertifiedGain) {
        return false;
    }

    //The certified point: gain at the intersection infimum, the other
    //parameters at the corner (thesis 4.3: the solution is a POINT; its
    //pseudocode substitutes into the whole box, an erratum).
    auto * numerador = new QVector<Parameter*>();
    foreach (qreal z, zeroSups) {
        numerador->append(new Parameter(z));
    }
    auto * denominador = new QVector<Parameter*>();
    foreach (qreal p, poleInfs) {
        denominador->append(new Parameter(p));
    }

    LtiSystem * point = box->create(box->name(), numerador, denominador,
                                    new Parameter(lowNeeded), new Parameter(qreal(0)));

    if (!boxIsFeasible(point) || !stability->isNominallyStable(point)) {
        delete point;
        return false;
    }

    bestCertifiedGain = lowNeeded;
    delete bestCertifiedController;
    bestCertifiedController = point;

    return true;
}


//------------------------------------------------------------------ QSFact
//Insertion of a certainly feasible box into the live list, guarded by the
//prune variable and the stability criterion.
inline void AlgorithmMcThesis::insertFeasibleBox(LtiSystem * box, Tripleta2 * parent)
{
    const qreal gainInf = box->gain()->range().x();

    if (gainInf > bestCertifiedGain) {
        delete box;
        return;
    }

    LtiSystem * point = guardarControlador(box, true);
    const bool stable = stability->isNominallyStable(point);

    if (!stable) {
        delete point;
        delete box;
        return;
    }

    //A feasible box also certifies its own gain infimum: it feeds C like
    //an MG solution (the thesis keeps both mechanisms; folding them keeps
    //one prune variable).
    if (gainInf < bestCertifiedGain) {
        bestCertifiedGain = gainInf;
        delete bestCertifiedController;
        bestCertifiedController = point;
    } else {
        delete point;
    }

    Tripleta2 * t = new Tripleta2(gainInf, box, feasible);
    t->setEtapas(parent->getEtapas());
    t->setRecorteActivado(false);
    t->setFrecuenciasFeasible(new QHash<qreal, qreal>());
    lista->insertar(t);
}


//QSFact (thesis 5.1.2): certainly feasible subranges of every parameter.
//For each parameter, each family (magnitude/phase) and each side of the
//range, every ambiguous frequency must certify a threshold with the
//closed-form equations at the corner that puts the loop CLOSEST to the
//boundary (the quantification runs over all values of the other
//parameters); frequencies where the box is feasible impose nothing. The
//intersection across frequencies (UM/UF) is split off into the live list
//and every valid per-frequency threshold is recorded for the tree
//bisection (MM/MF).
inline void AlgorithmMcThesis::feasibleCuts(Tripleta2 * node, const NodeAnalysis & analysis,
                                            QVector<FeasibleThreshold> & thresholds, bool & improved)
{
    LtiSystem * box = node->getSistema();
    const qint32 total = parameterCount(box);

    //family 0 = magnitude, 1 = phase; side true = upper subrange.
    for (qint32 parameter = 0; parameter < total; ++parameter) {

        const QPointF range = parameterRange(box, parameter);

        if (range.x() >= range.y()) {
            continue;   //fixed parameter
        }

        const bool isGain = parameter == 0;
        const bool isZero = !isGain && parameter <= box->numerator()->size();
        const qint32 termIndex = isGain ? -1
                : (isZero ? parameter - 1 : parameter - 1 - box->numerator()->size());

        for (qint32 family = 0; family < 2; ++family) {

            if (family == 0 && !strategies.feasibleMagnitude) {
                continue;
            }

            if (family == 1 && (isGain || !strategies.feasiblePhase)) {
                continue;
            }

            for (bool upperSide : {false, true}) {

                //Corner vectors are refreshed per attempt: earlier
                //extractions may have shrunk the box.
                QVector<qreal> zeroInfs, zeroSups, poleInfs, poleSups;
                cornerVectors(box, false, true, zeroInfs, poleSups);
                cornerVectors(box, true, false, zeroSups, poleInfs);
                const qreal kInf = box->gain()->range().x();
                const qreal kSup = box->gain()->range().y();

                qreal intersection = upperSide
                        ? std::numeric_limits<qreal>::lowest()
                        : std::numeric_limits<qreal>::max();
                bool allCertified = true;

                for (qint32 i = 0; i < omega->size() && allCertified; ++i) {

                    data_box * datos = analysis.datos.value(i);

                    if (datos == nullptr || datos->getFlag() != ambiguous) {
                        continue;   //feasible here for the whole range
                    }

                    const qreal w = omega->at(i);
                    const std::complex<qreal> p0 = plantas_nominales_std->at(i);

                    qreal t = -1.0;

                    if (family == 0) {
                        const qreal boundMin = std::pow(10.0, datos->getMinimoxMaximos()->at(0) / 20.0);
                        const qreal boundMax = std::pow(10.0, datos->getMinimoxMaximos()->at(1) / 20.0);

                        //Which boundary side must be feasible follows the
                        //parameter's monotonicity: gain and zeros raise
                        //the loop, poles lower it (the upper subrange of
                        //a pole lives on the bottom strip).
                        const bool topStrip = (isGain || isZero) ? upperSide : !upperSide;

                        if (topStrip) {
                            if (datos->isUniDerecha()) {   //top strip forbidden
                                allCertified = false;
                                break;
                            }
                            //Others at the loop-minimising corner.
                            if (isGain) {
                                t = quick_solution::gainCut(boundMax, zeroInfs, poleSups, w, p0);
                            } else if (isZero) {
                                t = quick_solution::zeroCut(boundMax, kInf, zeroInfs, poleSups, termIndex, w, p0);
                            } else {
                                t = quick_solution::poleCut(boundMax, kInf, zeroInfs, poleSups, termIndex, w, p0);
                            }
                        } else {
                            if (datos->isUniAbajo()) {     //bottom strip forbidden
                                allCertified = false;
                                break;
                            }
                            //Others at the loop-maximising corner.
                            if (isGain) {
                                t = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);
                            } else if (isZero) {
                                t = quick_solution::zeroCut(boundMin, kSup, zeroSups, poleInfs, termIndex, w, p0);
                            } else {
                                t = quick_solution::poleCut(boundMin, kSup, zeroSups, poleInfs, termIndex, w, p0);
                            }
                        }
                    } else {
                        const qreal phi0 = nominalPhase(p0);
                        const qreal thetaMin = datos->getMinimoxMaximos()->at(2) * M_PI / 180.0;
                        const qreal thetaMax = datos->getMinimoxMaximos()->at(3) * M_PI / 180.0;
                        const QPointF boxPhase = analysis.boxPhase.at(i);

                        //Zeros lower the phase as they grow, poles raise
                        //it: the upper subrange of a zero lives on the
                        //LEFT strip, of a pole on the RIGHT strip.
                        const bool rightStrip = isZero ? !upperSide : upperSide;

                        if (rightStrip) {
                            if (datos->isUniDerecha() ||
                                    datos->getMinimoxMaximos()->at(3) >= boxPhase.y() - phaseGridStep) {
                                allCertified = false;
                                break;
                            }
                            t = isZero
                                ? quick_solution::zeroPhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, termIndex, w)
                                : quick_solution::polePhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, termIndex, w);
                        } else {
                            if (datos->isUniIzquierda() ||
                                    datos->getMinimoxMaximos()->at(2) <= boxPhase.x() + phaseGridStep) {
                                allCertified = false;
                                break;
                            }
                            t = isZero
                                ? quick_solution::zeroPhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, termIndex, w)
                                : quick_solution::polePhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, termIndex, w);
                        }
                    }

                    if (t < 0.0) {
                        allCertified = false;
                        break;
                    }

                    //A threshold beyond the range on the certifying side
                    //imposes nothing at this frequency; one beyond the
                    //other side leaves no feasible subrange.
                    if (upperSide) {
                        if (t >= range.y()) {
                            allCertified = false;
                            break;
                        }
                        const qreal clamped = std::max(t, range.x());
                        intersection = std::max(intersection, clamped);

                        if (t > range.x()) {
                            thresholds.append({parameter, i, t, true,
                                               (range.y() - t) / (range.y() - range.x())});
                        }
                    } else {
                        if (t <= range.x()) {
                            allCertified = false;
                            break;
                        }
                        const qreal clamped = std::min(t, range.y());
                        intersection = std::min(intersection, clamped);

                        if (t < range.y()) {
                            thresholds.append({parameter, i, t, false,
                                               (t - range.x()) / (range.y() - range.x())});
                        }
                    }
                }

                if (!allCertified) {
                    continue;
                }

                //Strictly interior intersection: split the feasible
                //subrange off into the live list (UM/UF) and keep the
                //ambiguous remainder in the node.
                if (intersection <= range.x() || intersection >= range.y()) {
                    continue;
                }

                const QPointF feasiblePart = upperSide
                        ? QPointF(intersection, range.y())
                        : QPointF(range.x(), intersection);
                const QPointF ambiguousPart = upperSide
                        ? QPointF(range.x(), intersection)
                        : QPointF(intersection, range.y());

                LtiSystem * um = replaceParameter(box, parameter, feasiblePart);

                //Defensive verification with the real detection before
                //trusting the closed-form certificate.
                if (!boxIsFeasible(um)) {
                    delete um;
                    continue;
                }

                insertFeasibleBox(um, node);

                LtiSystem * remainder = replaceParameter(box, parameter, ambiguousPart);
                delete box;
                node->setSistema(remainder);
                box = remainder;

                improved = true;
            }
        }
    }
}


//------------------------------------------------------------------- QSInv
//QSInv (thesis 5.1.1): certainly infeasible subranges cut away, on every
//side of the projected box the corner classification certifies as
//forbidden: the magnitude cuts of NK's Quick Solution (bottom strip, plus
//their mirror on the top strip) and the phase cuts of thesis 4.1.2. All
//cuts run sequentially on the latest updated values.
inline void AlgorithmMcThesis::infeasibleCuts(Tripleta2 * node, const NodeAnalysis & analysis,
                                              bool & improved)
{
    LtiSystem * v = node->getSistema();

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
    qreal gainSup = v->gain()->range().y();

    bool cut = false;

    for (qint32 i = 0; i < omega->size(); ++i) {

        data_box * datos = analysis.datos.value(i);

        if (datos == nullptr || datos->getFlag() != ambiguous) {
            continue;
        }

        const qreal w = omega->at(i);
        const std::complex<qreal> p0 = plantas_nominales_std->at(i);
        const qreal boundMin = std::pow(10.0, datos->getMinimoxMaximos()->at(0) / 20.0);
        const qreal boundMax = std::pow(10.0, datos->getMinimoxMaximos()->at(1) / 20.0);

        //Bottom strip certainly forbidden: cuts from below (NK's QS).
        if (strategies.infeasibleMagnitude && datos->isUniAbajo()) {

            if (v->gain()->isUncertain()) {
                const qreal k = quick_solution::gainCut(boundMin, zeroSups, poleInfs, w, p0);
                if (k > gainInf && k < gainSup) {
                    gainInf = k;
                    cut = true;
                }
            }

            for (qint32 j = 0; hasUncertainZeros && j < zeroInfs.size(); ++j) {
                if (!v->numerator()->at(j)->isUncertain()) continue;
                const qreal z = quick_solution::zeroCut(boundMin, gainSup, zeroSups, poleInfs, j, w, p0);
                if (z > zeroInfs.at(j) && z < zeroSups.at(j)) {
                    zeroInfs.replace(j, z);
                    cut = true;
                }
            }

            for (qint32 j = 0; hasUncertainPoles && j < poleInfs.size(); ++j) {
                if (!v->denominator()->at(j)->isUncertain()) continue;
                const qreal p = quick_solution::poleCut(boundMin, gainSup, zeroSups, poleInfs, j, w, p0);
                if (p > poleInfs.at(j) && p < poleSups.at(j)) {
                    poleSups.replace(j, p);
                    cut = true;
                }
            }
        }

        //Top strip certainly forbidden: the mirror cuts from above, with
        //the loop-minimising corner and B_max.
        if (strategies.infeasibleMagnitude && datos->isUniDerecha()) {

            if (v->gain()->isUncertain()) {
                const qreal k = quick_solution::gainCut(boundMax, zeroInfs, poleSups, w, p0);
                if (k > gainInf && k < gainSup) {
                    gainSup = k;
                    cut = true;
                }
            }

            for (qint32 j = 0; hasUncertainZeros && j < zeroInfs.size(); ++j) {
                if (!v->numerator()->at(j)->isUncertain()) continue;
                const qreal z = quick_solution::zeroCut(boundMax, gainInf, zeroInfs, poleSups, j, w, p0);
                if (z > zeroInfs.at(j) && z < zeroSups.at(j)) {
                    zeroSups.replace(j, z);
                    cut = true;
                }
            }

            for (qint32 j = 0; hasUncertainPoles && j < poleInfs.size(); ++j) {
                if (!v->denominator()->at(j)->isUncertain()) continue;
                const qreal p = quick_solution::poleCut(boundMax, gainInf, zeroInfs, poleSups, j, w, p0);
                if (p > poleInfs.at(j) && p < poleSups.at(j)) {
                    poleInfs.replace(j, p);
                    cut = true;
                }
            }
        }

        //Phase strips (thesis 4.1.2), when wider than one grid step.
        if (strategies.infeasiblePhase && (hasUncertainZeros || hasUncertainPoles)) {

            const qreal phi0 = nominalPhase(p0);
            const QPointF boxPhase = analysis.boxPhase.at(i);
            const qreal boundPhaseMin = datos->getMinimoxMaximos()->at(2);
            const qreal boundPhaseMax = datos->getMinimoxMaximos()->at(3);

            if (datos->isUniDerecha() && boundPhaseMax < boxPhase.y() - phaseGridStep) {

                const qreal thetaMax = boundPhaseMax * M_PI / 180.0;

                for (qint32 j = 0; hasUncertainZeros && j < zeroInfs.size(); ++j) {
                    if (!v->numerator()->at(j)->isUncertain()) continue;
                    const qreal z = quick_solution::zeroPhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, j, w);
                    if (z > zeroInfs.at(j) && z < zeroSups.at(j)) {
                        zeroInfs.replace(j, z);
                        cut = true;
                    }
                }

                for (qint32 j = 0; hasUncertainPoles && j < poleInfs.size(); ++j) {
                    if (!v->denominator()->at(j)->isUncertain()) continue;
                    const qreal p = quick_solution::polePhaseCutHigh(thetaMax, phi0, zeroSups, poleInfs, j, w);
                    if (p > poleInfs.at(j) && p < poleSups.at(j)) {
                        poleSups.replace(j, p);
                        cut = true;
                    }
                }
            }

            if (datos->isUniIzquierda() && boundPhaseMin > boxPhase.x() + phaseGridStep) {

                const qreal thetaMin = boundPhaseMin * M_PI / 180.0;

                for (qint32 j = 0; hasUncertainZeros && j < zeroInfs.size(); ++j) {
                    if (!v->numerator()->at(j)->isUncertain()) continue;
                    const qreal z = quick_solution::zeroPhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, j, w);
                    if (z > zeroInfs.at(j) && z < zeroSups.at(j)) {
                        zeroSups.replace(j, z);
                        cut = true;
                    }
                }

                for (qint32 j = 0; hasUncertainPoles && j < poleInfs.size(); ++j) {
                    if (!v->denominator()->at(j)->isUncertain()) continue;
                    const qreal p = quick_solution::polePhaseCutLow(thetaMin, phi0, zeroInfs, poleSups, j, w);
                    if (p > poleInfs.at(j) && p < poleSups.at(j)) {
                        poleInfs.replace(j, p);
                        cut = true;
                    }
                }
            }
        }
    }

    if (!cut) {
        return;
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
    node->setSistema(nuevo);
    improved = true;
}


//-------------------------------------------------------------- bisection
//Split one parameter at 'point'; both children inherit the node's stage,
//cut switch and feasible-frequency history.
inline FC::return_bisection2 AlgorithmMcThesis::bisectAt(Tripleta2 * node, qint32 parameter,
                                                         qreal point)
{
    LtiSystem * box = node->getSistema();
    const QPointF range = parameterRange(box, parameter);

    LtiSystem * lower = replaceParameter(box, parameter, QPointF(range.x(), point));
    LtiSystem * upper = replaceParameter(box, parameter, QPointF(point, range.y()));

    const auto makeChild = [&](LtiSystem * system) {
        Tripleta2 * t = new Tripleta2(system->gain()->range().x(), system, ambiguous);
        t->setEtapas(node->getEtapas());
        t->setRecorteActivado(node->isRecorteActivado());
        t->setFrecuenciasFeasible(node->getFrecuenciasFeasible() != nullptr
                ? new QHash<qreal, qreal>(*node->getFrecuenciasFeasible())
                : new QHash<qreal, qreal>());
        return t;
    };

    FC::return_bisection2 retur;
    retur.t1 = makeChild(lower);
    retur.t2 = makeChild(upper);
    retur.descartado = false;

    destroyNode(node);

    return retur;
}


//The parameter whose Nichols term box at the main frequency contributes
//most by the requested measure (0 = area, 1 = magnitude, 2 = phase).
//The gain has no phase component, so measure 2 skips it and measures 0/1
//use its magnitude width (its phase width is zero).
inline qint32 AlgorithmMcThesis::widestByMeasure(Tripleta2 * node, qint32 mainFrequency, int measure)
{
    LtiSystem * box = node->getSistema();
    const qreal w = omega->at(mainFrequency);
    const cxsc::complex p0 = plantas_nominales->at(mainFrequency);

    qint32 best = -1;
    qreal bestValue = -1.0;

    const auto consider = [&](qint32 parameter, const cinterval & term, bool gainTerm) {
        qreal value;

        if (measure == 2) {
            if (gainTerm) {
                return;
            }
            value = _double(diam(Im(term)));
        } else if (measure == 1 || gainTerm) {
            value = _double(diam(Re(term)));
        } else {
            value = _double(diam(Re(term))) * _double(diam(Im(term)));
        }

        if (value > bestValue) {
            bestValue = value;
            best = parameter;
        }
    };

    if (box->gain()->isUncertain()) {
        consider(0, conversion->gainTermBox(box->gain(), p0), true);
    }

    for (qint32 j = 0; j < box->numerator()->size(); ++j) {
        if (box->numerator()->at(j)->isUncertain()) {
            consider(j + 1, conversion->numeratorTermBox(box->numerator()->at(j), w, p0), false);
        }
    }

    for (qint32 j = 0; j < box->denominator()->size(); ++j) {
        if (box->denominator()->at(j)->isUncertain()) {
            consider(j + 1 + box->numerator()->size(),
                     conversion->denominatorTermBox(box->denominator()->at(j), w, p0), false);
        }
    }

    return best;
}


//Step G (thesis 5.4.6): the bisection strategy follows the node's stage.
inline FC::return_bisection2 AlgorithmMcThesis::bisect(Tripleta2 * node, const NodeAnalysis & analysis,
                                                       const QVector<FeasibleThreshold> & thresholds)
{
    //Tree bisection (thesis 5.3): split at the stored feasible threshold
    //covering the largest fraction of its parameter's current range, and
    //mark the feasible child for that frequency.
    if (strategies.treeBisection &&
            node->getEtapas() == Etapas::INTERMEDIA && !thresholds.isEmpty()) {

        const FeasibleThreshold * bestThreshold = nullptr;
        qreal bestFraction = 0.0;

        foreach (const FeasibleThreshold & t, thresholds) {
            const QPointF range = parameterRange(node->getSistema(), t.parameter);

            if (t.threshold <= range.x() || t.threshold >= range.y()) {
                continue;   //the range moved since the threshold was recorded
            }

            const qreal fraction = t.upperSide
                    ? (range.y() - t.threshold) / (range.y() - range.x())
                    : (t.threshold - range.x()) / (range.y() - range.x());

            if (fraction > bestFraction) {
                bestFraction = fraction;
                bestThreshold = &t;
            }
        }

        if (bestThreshold != nullptr) {
            const qint32 freq = bestThreshold->freqIndex;
            FC::return_bisection2 retur = bisectAt(node, bestThreshold->parameter,
                                                   bestThreshold->threshold);
            Tripleta2 * feasibleChild = bestThreshold->upperSide ? retur.t2 : retur.t1;

            //Defensive verification of the mark with the real detection:
            //an unverified mark would silently skip this frequency's
            //feasibility test in the whole subtree.
            if (boxIsFeasibleAt(feasibleChild->getSistema(), freq)) {
                feasibleChild->addFrecuenciaFeasible(freq, omega->at(freq));
            }

            return retur;
        }
    }

    //Stage-driven measure: area in the initial stage (and as the general
    //fallback), the wider of magnitude/phase in the final stage.
    int measure = 0;

    if (node->getEtapas() == Etapas::FINAL) {
        const QPointF mag = analysis.boxMag.at(analysis.mainFrequency);
        const QPointF fas = analysis.boxPhase.at(analysis.mainFrequency);
        measure = (fas.y() - fas.x()) > (mag.y() - mag.x()) ? 2 : 1;
    }

    qint32 parameter = widestByMeasure(node, analysis.mainFrequency, measure);

    if (parameter < 0) {
        parameter = widestByMeasure(node, analysis.mainFrequency, 0);
    }

    const QPointF range = parameterRange(node->getSistema(), parameter);

    return bisectAt(node, parameter, range.x() + (range.y() - range.x()) / 2.0);
}
