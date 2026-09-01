#include "free_form.h"

#include <cmath>
#include <complex>

#include <QRegularExpression>

#include "src/core/exception.h"

using namespace std;
using namespace mup;

namespace qftbx {

FreeForm::FreeForm(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k,
                           Parameter delay, QString numeratorExpr, QString denominatorExpr)
    :TransferFunction (name, std::move(numerator), std::move(denominator), std::move(k), std::move(delay))
{
    m_numeratorExpr = numeratorExpr;
    m_denominatorExpr = denominatorExpr;
}

//Evaluation with explicit parameter values needs the free-form expression
//rebuilt around those values, which no consumer requires any more (the
//loop-shaping cuts moved to closed forms over zero-pole-gain structures).
//The historical stubs returned 0 SILENTLY, poisoning any computation that
//reached them; failing loudly keeps a future caller honest.
std::complex <qreal> FreeForm::evaluate (QVector <qreal> *, QVector <qreal> *,
                                             qreal, qreal, qreal){
    throw ComputationError("FreeForm: evaluation with explicit parameter "
                           "values is not implemented for free-form systems.");
}

QString FreeForm::expression (QVector <qreal> *, QVector <qreal> *,
                               qreal, qreal, qreal){
    throw ComputationError("FreeForm: the expression with explicit parameter "
                           "values is not implemented for free-form systems.");
}

std::complex <qreal> FreeForm::evaluateNumerator(QVector <qreal> *, qreal){
    throw ComputationError("FreeForm: numerator evaluation with explicit "
                           "values is not implemented for free-form systems.");
}

std::complex <qreal> FreeForm::evaluateDenominator(QVector <qreal> *, qreal){
    throw ComputationError("FreeForm: denominator evaluation with explicit "
                           "values is not implemented for free-form systems.");
}

QString FreeForm::expression(qreal w){

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

LtiSystem * FreeForm::create(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                               Parameter k, Parameter delay, QString numeratorExpr, QString denominatorExpr){

    return new FreeForm (name, std::move(numerator), std::move(denominator), std::move(k),
                         std::move(delay), numeratorExpr, denominatorExpr);
}


QString FreeForm::numeratorString(){
    return m_numeratorExpr;
}

QString FreeForm::denominatorString(){
    return m_denominatorExpr;
}

LtiSystem * FreeForm::clone(){

    return this->create(this->name(), m_numerator, m_denominator, m_gain, m_delay,
                        m_numeratorExpr, m_denominatorExpr);
}

} // namespace qftbx

//A free-form plant is written by the user, so its numerator and denominator
//have to be evaluated as expressions - but neither the frequency nor the
//coefficients need to travel as TEXT. The Laplace variable is replaced by a
//BOUND variable and the named coefficients are bound to their values, so
//nothing goes through QString::number() and its six significant digits. The
//gain and the delay arrive already reduced to values (Parameter::nominal()
//has applied any reparametrisation), so their own expressions are not
//re-evaluated here either.
std::complex <qreal> FreeForm::valueAt(qreal w, const std::vector<qreal> & numerator,
                                       const std::vector<qreal> & denominator,
                                       qreal gain, qreal delay)
{
    //Only the standalone Laplace variable becomes the bound one: a plain
    //substring replace mutilated "sin", "sqrt", "abs" and any parameter whose
    //name contains an 's'.
    static const QRegularExpression laplaceVariable(QStringLiteral("\\bs\\b"));
    static const QString laplaceName = QStringLiteral("__jw");

    QString numeratorText = m_numeratorExpr;
    QString denominatorText = m_denominatorExpr;
    numeratorText.replace(laplaceVariable, laplaceName);
    denominatorText.replace(laplaceVariable, laplaceName);

    //One value per DISTINCT name. A name appearing more than once is ONE
    //variable, not several - the cervera plant carries its "a" in both the
    //numerator and the denominator - so every appearance must be given the
    //same value, and DefineVar would throw on the second one anyway. A
    //disagreement means the caller built an inconsistent request; picking one
    //of the two would evaluate a plant nobody asked for, so it is reported.
    QVector<QString> names;
    QVector<qreal> bound;

    const auto remember = [&](std::vector<Parameter> & parameters, const std::vector<qreal> & given) {
        for (std::size_t i = 0; i < parameters.size() && i < given.size(); i++) {
            const QString name = parameters[i].name();
            const qint32 at = names.indexOf(name);

            if (at < 0) {
                names.append(name);
                bound.append(given[i]);
                continue;
            }

            if (bound.at(at) != given[i]) {
                throw qftbx::InvalidInput(
                    QString("the parameter \"%1\" was given two different values "
                            "(%2 and %3): the same name is the same variable")
                        .arg(name).arg(bound.at(at)).arg(given[i]).toStdString());
            }
        }
    };

    remember(m_numerator, numerator);
    remember(m_denominator, denominator);

    //Values before the parser and SIZED UP FRONT: Variable stores a POINTER
    //to them, so a vector that reallocated would dangle every earlier bind,
    //and destruction runs in reverse declaration order.
    Value laplace (std::complex<qreal>(0.0, w));
    std::vector<Value> values (static_cast<std::size_t>(bound.size()));

    ParserX parser (pckALL_COMPLEX);
    parser.DefineVar(laplaceName.toStdString(), Variable(&laplace));

    for (qint32 i = 0; i < names.size(); i++) {
        values[static_cast<std::size_t>(i)] = Value(bound.at(i));
        parser.DefineVar(names.at(i).toStdString(),
                         Variable(&values[static_cast<std::size_t>(i)]));
    }

    const QString expr = "(" + numeratorText + ")/(" + denominatorText + ")";
    parser.SetExpr(expr.toStdString());

    const std::complex<qreal> s(0.0, w);

    return gain * parser.Eval().GetComplex() * std::exp(-s * delay);
}
