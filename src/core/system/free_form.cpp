#include <algorithm>
#include <vector>
#include <cstdint>
#include "free_form.h"

#include <cmath>
#include <complex>

#include <QRegularExpression>

#include "src/core/exception.h"
#include "src/core/math/expression_cache.h"

using namespace std;
using namespace mup;

namespace qftbx {

FreeForm::FreeForm(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k,
                           Parameter delay, QString numeratorExpr, QString denominatorExpr)
    :TransferFunction (name, std::move(numerator), std::move(denominator), std::move(k), std::move(delay))
{
    m_numeratorExpr = numeratorExpr;
    m_denominatorExpr = denominatorExpr;

    //Built HERE and never again. It was a lazily filled mutable member, and
    //valueAt() runs on one plant from several OpenMP threads: two of them saw
    //it empty, both built it and both assigned a QString - a refcounted
    //pointer swap - which ThreadSanitizer caught as a data race. Doing it
    //once in the constructor costs two regular expressions per plant and
    //leaves nothing shared to race on.
    //
    //Only the standalone Laplace variable is replaced: a plain substring
    //replace mutilated "sin", "sqrt", "abs" and any parameter whose name
    //contains an 's'.
    static const QRegularExpression laplaceVariable(QStringLiteral("\\bs\\b"));

    QString numeratorText = m_numeratorExpr;
    QString denominatorText = m_denominatorExpr;
    numeratorText.replace(laplaceVariable, laplaceName());
    denominatorText.replace(laplaceVariable, laplaceName());

    m_boundExpression = "(" + numeratorText + ")/(" + denominatorText + ")";
}

//Evaluation with explicit parameter values needs the free-form expression
//rebuilt around those values, which no consumer requires any more (the
//loop-shaping cuts moved to closed forms over zero-pole-gain structures).
//The historical stubs returned 0 SILENTLY, poisoning any computation that
//reached them; failing loudly keeps a future caller honest.
std::complex <double> FreeForm::evaluate (std::vector <double> *, std::vector <double> *,
                                             double, double, double){
    throw ComputationError("FreeForm: evaluation with explicit parameter "
                           "values is not implemented for free-form systems.");
}

QString FreeForm::expression (std::vector <double> *, std::vector <double> *,
                               double, double, double){
    throw ComputationError("FreeForm: the expression with explicit parameter "
                           "values is not implemented for free-form systems.");
}

std::complex <double> FreeForm::evaluateNumerator(std::vector <double> *, double){
    throw ComputationError("FreeForm: numerator evaluation with explicit "
                           "values is not implemented for free-form systems.");
}

std::complex <double> FreeForm::evaluateDenominator(std::vector <double> *, double){
    throw ComputationError("FreeForm: denominator evaluation with explicit "
                           "values is not implemented for free-form systems.");
}

QString FreeForm::expression(double w){

    //Only the standalone Laplace variable becomes jw: a plain substring
    //replace mutilated "sin", "sqrt", "abs" and any parameter whose name
    //contains an 's'.
    const QRegularExpression laplaceVariable(QStringLiteral("\\bs\\b"));
    const QString jw = "(" + QString::number(w) + "*i)";

    QString n = m_numeratorExpr;
    QString d = m_denominatorExpr;

    QString expr = m_gain.expression() + "*(" + n.replace(laplaceVariable, jw) + ")/(" +
            d.replace(laplaceVariable, jw) + ")";


    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal, so the template sweep can drive
    //it) or a non-zero constant.
    if (m_delay.isUncertain()){
        expr += "* e^(-i*" + QString::number(w) + "*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-i*" + QString::number(w) + "*" +
                QString::number(m_delay.nominal()) +")";
    }

    return expr;
}

QString FreeForm::expression(){
    QString expr = m_gain.expression() + "*(" + m_numeratorExpr + ")/(" + m_denominatorExpr + ")";

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + QString::number(m_delay.nominal()) +")";
    }

    return expr;
}

LtiSystem::SystemType FreeForm::type(){
    return SystemType::FreeForm;
}

std::unique_ptr<LtiSystem> FreeForm::create(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                               Parameter k, Parameter delay, QString numeratorExpr, QString denominatorExpr){

    return std::make_unique<FreeForm>(name, std::move(numerator), std::move(denominator),
                                     std::move(k), std::move(delay), numeratorExpr,
                                     denominatorExpr);
}


QString FreeForm::numeratorString(){
    return m_numeratorExpr;
}

QString FreeForm::denominatorString(){
    return m_denominatorExpr;
}

std::unique_ptr<LtiSystem> FreeForm::clone(){

    return this->create(this->name(), m_numerator, m_denominator, m_gain, m_delay,
                        m_numeratorExpr, m_denominatorExpr);
}

//A free-form plant is written by the user, so its numerator and denominator
//have to be evaluated as expressions - but neither the frequency nor the
//coefficients need to travel as TEXT. The Laplace variable is replaced by a
//BOUND variable and the named coefficients are bound to their values, so
//nothing goes through QString::number() and its six significant digits. The
//gain and the delay arrive already reduced to values (Parameter::nominal()
//has applied any reparametrisation), so their own expressions are not
//re-evaluated here either.
std::complex <double> FreeForm::valueAt(double w, const std::vector<double> & numerator,
                                       const std::vector<double> & denominator,
                                       double gain, double delay)
{
    //One value per DISTINCT name. A name appearing more than once is ONE
    //variable, not several - the cervera plant carries its "a" in both the
    //numerator and the denominator - so every appearance must be given the
    //same value. A disagreement means the caller built an inconsistent
    //request; picking one of the two would evaluate a plant nobody asked
    //for, so it is reported.
    std::vector<QString> names;
    std::vector<std::complex<double>> bound;

    names.push_back(laplaceName());
    bound.push_back(std::complex<double>(0.0, w));

    const auto remember = [&](std::vector<Parameter> & parameters, const std::vector<double> & given) {
        for (std::size_t i = 0; i < parameters.size() && i < given.size(); i++) {
            const QString name = parameters[i].name();
            const auto found = std::find(names.begin(), names.end(), name);
            const std::int32_t at = found == names.end()
                    ? -1
                    : static_cast<std::int32_t>(std::distance(names.begin(), found));

            if (at < 0) {
                names.push_back(name);
                bound.push_back(std::complex<double>(given[i], 0.0));
                continue;
            }

            if (bound[static_cast<std::size_t>(at)] != std::complex<double>(given[i], 0.0)) {
                throw qftbx::InvalidInput(
                    QString("the parameter \"%1\" was given two different values "
                            "(%2 and %3): the same name is the same variable")
                        .arg(name).arg(bound[static_cast<std::size_t>(at)].real()).arg(given[i])
                        .toStdString());
            }
        }
    };

    remember(m_numerator, numerator);
    remember(m_denominator, denominator);

    //Parsed once per thread: see qftbx::math::evaluateCached. The text no
    //longer carries the frequency, so it is the same expression on every
    //call and the cache actually hits.
    const std::complex<double> ratio =
            qftbx::math::evaluateCached(m_boundExpression, names, bound);

    const std::complex<double> s(0.0, w);

    return gain * ratio * std::exp(-s * delay);
}

//The Laplace variable as a BOUND variable name. Substituting the frequency as
//text was what rounded it to six significant digits.
const QString & FreeForm::laplaceName()
{
    static const QString name = QStringLiteral("__jw");
    return name;
}

} // namespace qftbx
