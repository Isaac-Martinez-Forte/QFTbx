#include "src/core/exception.h"
#include "src/core/loopshaping/algorithm_mr.h"

#include "src/core/specifications/specification_record.h"

#include <cmath>

using namespace tools;
using namespace cxsc;
using namespace FC;
using namespace alg;

namespace {

//Template representatives per frequency entering the constraint set (the
//paper uses 9 plants; the tracking constraints pair them quadratically).
const qint32 kTemplateRepresentatives = 9;

//Passes of the HC4 fixpoint loop per box (a bound protects against
//oscillating contractions; convergence is typically immediate).
const qint32 kMaxNarrowingPasses = 8;

QString number(qreal value)
{
    //Full precision; the expression lexer understands scientific
    //notation (fixed notation truncated the small tracking coefficients
    //to zero).
    return QString::number(value, 'g', 17);
}

} // namespace

AlgorithmMr::AlgorithmMr()
{
}

AlgorithmMr::~AlgorithmMr()
{
}

void AlgorithmMr::set_datos(LtiSystem *planta, LtiSystem *controlador, QVector<qreal> * omega, BoundaryData *boundaries,
                                  qreal epsilon, const qftbx::CloudSet & temp, QVector<qftbx::SpecificationRecord *> * espe){
    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
    this->temp = temp;
    this->espe = espe;
}


//Controller magnitude and phase as expression strings over the uncertain
//parameter names, one pair per design frequency. A zero-pole-gain factor
//(jw + x) contributes sqrt(x^2 + w^2) to the magnitude; a time-constant
//factor (1 + jw/x) contributes sqrt(1 + w^2/x^2); both contribute
//atan(w/x) to the phase in radians (the historical builder emitted
//atan(x/w), the complement of the true phase).
inline void AlgorithmMr::buildControllerExpressions(){

    const bool timeConstant =
            controlador->type() == LtiSystem::SystemType::TimeConstantGain;

    if (!timeConstant && controlador->type() != LtiSystem::SystemType::ZeroPoleGain) {
        throw qftbx::InvalidInput(
                "The ICSP loop-shaping algorithm needs a zero-pole-gain or "
                "time-constant controller structure.");
    }

    const auto term = [&](Parameter & var, qreal w) -> QString {
        const QString value = var.isUncertain() ? var.name() : number(var.nominal());
        if (timeConstant) {
            return "sqrt(1+(" + number(w * w) + "/(" + value + "^2)))";
        }
        return "sqrt((" + value + "^2)+" + number(w * w) + ")";
    };

    const auto phaseTerm = [&](Parameter & var, qreal w) -> QString {
        const QString value = var.isUncertain() ? var.name() : number(var.nominal());
        return "atan(" + number(w) + "/(" + value + "))";
    };

    const QString gain = controlador->gain().isUncertain()
            ? controlador->gain().name()
            : number(controlador->gain().nominal());

    magnitudeExpressions.clear();
    phaseExpressions.clear();

    foreach (qreal w, *omega) {

        QString magnitude = "(" + gain + ")";
        QString phase = "(0";

        for (Parameter & var : controlador->numerator()) {
            magnitude += "*" + term(var, w);
            phase += "+" + phaseTerm(var, w);
        }

        for (Parameter & var : controlador->denominator()) {
            magnitude += "/" + term(var, w);
            phase += "-" + phaseTerm(var, w);
        }

        phase += ")";

        magnitudeExpressions.append(magnitude);
        phaseExpressions.append(phase);
    }
}


//The constraint set of the ICSP (paper eqs. (10)-(11) plus the analogous
//QFTbx quadratics for the remaining specifications), one inequality
//"expression >= 0" per template representative (pairs for tracking) and
//design frequency where the specification band applies.
inline void AlgorithmMr::buildConstraints(){

    //The validated specification set, the same accessor the boundary
    //engine cuts at (the raw record heightDb evaluated NaN on some legacy
    //system specifications).
    const qftbx::SpecificationSet specifications = qftbx::toSpecificationSet(*espe);

    const auto applies = [&](qint32 slot, qreal w) {
        return specifications.at(static_cast<qftbx::SpecificationType>(slot)).appliesAt(w);
    };

    const auto boundDb = [&](qint32 slot, qreal w) {
        return specifications.at(static_cast<qftbx::SpecificationType>(slot)).boundDb(w);
    };

    const auto addConstraint = [&](const QString & expression) {
        ExpressionTree * tree = new ExpressionTree("1");
        tree->setFunc(expression.toStdString(), 0.0, alg::MAYORIGUAL);
        constraints.append(tree);
        constraintTexts.append(expression);
    };

    for (qint32 i = 0; i < omega->size(); ++i) {

        const qreal w = omega->at(i);
        const QString & g = magnitudeExpressions.at(i);
        const QString & phi = phaseExpressions.at(i);

        //Template representatives, evenly subsampled along the contour.
        //Non-finite or null points (artefacts of a degenerate contour)
        //would embed "nan" into the expression texts: they are skipped.
        QVector<std::complex<qreal>> points;
        const qftbx::ComplexCloud & contour = temp.at(static_cast<std::size_t>(i));
        const qint32 take = std::min<qint32>(kTemplateRepresentatives, static_cast<qint32>(contour.size()));
        for (qint32 j = 0; j < take; ++j) {
            const std::complex<qreal> value = contour.at(j * static_cast<qint32>(contour.size()) / take);
            if (std::isfinite(value.real()) && std::isfinite(value.imag()) &&
                    std::abs(value) > 0.0) {
                points.append(value);
            }
        }

        foreach (const std::complex<qreal> & value, points) {

            const QString p = number(std::abs(value));
            const QString p2 = number(std::abs(value) * std::abs(value));
            const QString theta = number(std::arg(value));

            //|1 + L|^2 expanded: g^2 p^2 + 2 g p cos(phi + theta) + 1.
            const QString l2 = "((" + g + ")^2)*(" + p2 + ")+2*(" + g + ")*(" + p +
                    ")*cos((" + phi + ")+(" + theta + "))+1";

            //Stability margin |T| <= ws (paper eq. (10)); the sensor noise
            //specification shares the same transfer.
            for (qint32 slot : {2, 3}) {
                if (applies(slot, w)) {
                    const qreal ws = std::pow(10.0, boundDb(slot, w) / 20.0);
                    addConstraint("((" + g + ")^2)*(" + p2 + ")*(1-" +
                                  number(1.0 / (ws * ws)) + ")+2*(" + g + ")*(" + p +
                                  ")*cos((" + phi + ")+(" + theta + "))+1");
                }
            }

            //Output disturbance rejection |1/(1+L)| <= d:
            //|1+L|^2 - 1/d^2 >= 0.
            if (applies(4, w)) {
                const qreal d = std::pow(10.0, boundDb(4, w) / 20.0);
                addConstraint("(" + l2 + ")-" + number(1.0 / (d * d)));
            }

            //Input disturbance rejection |P/(1+L)| <= d:
            //|1+L|^2 - p^2/d^2 >= 0.
            if (applies(5, w)) {
                const qreal d = std::pow(10.0, boundDb(5, w) / 20.0);
                addConstraint("(" + l2 + ")-(" + p2 + ")*" + number(1.0 / (d * d)));
            }

            //Control effort |G/(1+L)| <= d: |1+L|^2 - g^2/d^2 >= 0 (the
            //historical rule dropped the g^2 factor).
            if (applies(6, w)) {
                const qreal d = std::pow(10.0, boundDb(6, w) / 20.0);
                addConstraint("(" + l2 + ")-((" + g + ")^2)*" + number(1.0 / (d * d)));
            }
        }

        //Tracking spread (paper eq. (11)) over ORDERED representative
        //pairs, with delta = |T_U/T_L| at this frequency.
        if (applies(0, w) && applies(1, w)) {

            const qreal deltaDb = boundDb(1, w) - boundDb(0, w);
            const qreal delta2 = std::pow(10.0, deltaDb / 10.0);
            const QString invDelta2 = number(1.0 / delta2);

            for (qint32 a = 0; a < points.size(); ++a) {
                for (qint32 b = 0; b < points.size(); ++b) {
                    if (a == b) {
                        continue;
                    }

                    const qreal pi = std::abs(points.at(a));
                    const qreal thetaI = std::arg(points.at(a));
                    const qreal pk = std::abs(points.at(b));
                    const qreal thetaK = std::arg(points.at(b));

                    addConstraint("((" + g + ")^2)*" + number(pk * pk * pi * pi) +
                            "*(1-" + invDelta2 + ")+2*(" + g + ")*" + number(pk * pi) +
                            "*(" + number(pk) + "*cos((" + phi + ")+(" + number(thetaI) +
                            "))-" + number(pi) + "*" + invDelta2 + "*cos((" + phi +
                            ")+(" + number(thetaK) + ")))+" +
                            number(pk * pk) + "-" + number(pi * pi) + "*" + invDelta2);
                }
            }
        }
    }
}


bool AlgorithmMr::init_algorithm(){

    lista = new OrderedList();
    conversion = new NaturalIntervalExtension();
    stability = new NominalStabilityChecker(planta, omega);

    plantas_nominales = new QVector<cxsc::complex>();
    foreach (qreal o, *omega) {
        std::complex<qreal> c = planta->evaluate(o);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }

    buildControllerExpressions();
    buildConstraints();

    const auto cleanup = [this]() {
        delete conversion;
        delete lista;
        delete stability;
        delete plantas_nominales;
        qDeleteAll(constraints);
        constraints.clear();
    };

    classifyAndInsert(controlador);

    while (true) {

        if (lista->isEmpty()) {
            cleanup();
            throw qftbx::InvalidInput(
                    "No feasible solution exists in the given search box.");
        }

        SearchNode * node = static_cast<SearchNode *>(lista->first());
        lista->removeFirst();

        if (node->flag() == feasible || FC::isEpsilonSmall(
                    node->system(), epsilon, omega, conversion, plantas_nominales)) {

            controlador_retorno = pointFromBox(node->system(),
                                                     node->flag() != ambiguous);

            //Every returned point must be nominally stabilising.
            if (!stability->isNominallyStable(controlador_retorno)) {
                delete controlador_retorno;
                delete node;
                continue;
            }

            delete node;
            cleanup();
            return true;
        }

        struct BisectionResult retur = bisectWidestParameter(node->system());

        delete node;

        classifyAndInsert(retur.v1);
        classifyAndInsert(retur.v2);
    }
}


LtiSystem * AlgorithmMr::controllerStructure(){
    return controlador_retorno;
}


//Branch & prune step for one box: narrow the parameter domains with the
//HC4 filter over the whole constraint set; an emptied domain proves the
//box infeasible, and non-negative interval evaluations of every
//constraint prove it feasible.
inline void AlgorithmMr::classifyAndInsert(LtiSystem * box){

    std::map<std::string, cxsc::interval> domains;
    loadDomains(box, domains);

    if (!narrowToFixpoint(domains)) {
        delete box;
        return;
    }

    LtiSystem * narrowed = boxFromDomains(box, domains);
    delete box;

    const BoxFlag flag = certainlyFeasible(domains) ? feasible : ambiguous;

    lista->insert(new SearchNode(narrowed->gain().range().min, narrowed, flag));
}


inline bool AlgorithmMr::narrowToFixpoint(std::map<std::string, cxsc::interval> & domains){

    for (qint32 pass = 0; pass < kMaxNarrowingPasses; ++pass) {

        const std::map<std::string, cxsc::interval> snapshot = domains;

        foreach (ExpressionTree * tree, constraints) {
            if (!tree->propagate(&domains)) {
                return false;
            }
        }

        bool changed = false;
        for (auto it = domains.begin(); it != domains.end(); ++it) {
            const cxsc::interval previous = snapshot.at(it->first);
            if (Inf(it->second) != Inf(previous) || Sup(it->second) != Sup(previous)) {
                changed = true;
                break;
            }
        }

        if (!changed) {
            break;
        }
    }

    return true;
}


inline bool AlgorithmMr::certainlyFeasible(std::map<std::string, cxsc::interval> & domains){

    foreach (ExpressionTree * tree, constraints) {
        if (cxsc::_double(Inf(tree->eval(&domains))) < 0.0) {
            return false;
        }
    }

    return true;
}


inline void AlgorithmMr::loadDomains(LtiSystem * box,
                                           std::map<std::string, cxsc::interval> & domains){

    domains.clear();

    const auto load = [&](Parameter & var) {
        if (var.isUncertain()) {
            domains[var.name().toStdString()] =
                    cxsc::interval(var.range().min, var.range().max);
        }
    };

    for (Parameter & var : box->numerator()) {
        load(var);
    }
    for (Parameter & var : box->denominator()) {
        load(var);
    }
    load(box->gain());
}


inline LtiSystem * AlgorithmMr::boxFromDomains(LtiSystem * box,
                                                     const std::map<std::string, cxsc::interval> & domains){

    const auto rebuilt = [&](Parameter & var) -> Parameter {
        if (!var.isUncertain()) {
            return Parameter(var.nominal());
        }
        const cxsc::interval value = domains.at(var.name().toStdString());
        return Parameter(var.name(),
                         Range(cxsc::_double(Inf(value)), cxsc::_double(Sup(value))),
                         cxsc::_double(Inf(value)));
    };

    std::vector<Parameter> nume;
    nume.reserve(box->numerator().size());
    for (Parameter & var : box->numerator()) {
        nume.push_back(rebuilt(var));
    }

    std::vector<Parameter> deno;
    deno.reserve(box->denominator().size());
    for (Parameter & var : box->denominator()) {
        deno.push_back(rebuilt(var));
    }

    return box->create(box->name(), std::move(nume), std::move(deno),
                       rebuilt(box->gain()), Parameter(qreal(0)));
}
