#include "free_form.h"

#include <QRegularExpression>

#include "Modelo/Herramientas/exception.h"

using namespace std;
using namespace mup;

namespace qftbx {

FreeForm::FreeForm(QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator, Parameter * k,
                           Parameter* delay, QString numeratorExpr, QString denominatorExpr)
    :TransferFunction (name, numerator, denominator, k, delay)
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

    QString expr = m_gain->expression() + "*(" + n.replace(laplaceVariable, jw) + ")/(" +
            d.replace(laplaceVariable, jw) + ")";


    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal, so the template sweep can drive
    //it) or a non-zero constant.
    if (m_delay->isUncertain()){
        expr += "* e^(-i*" + QString::number(w) + "*" + m_delay->name() + ")";
    }else if (m_delay->nominal() != 0){
        expr += "* e^(-i*" + QString::number(w) + "*" +
                QString::number(m_delay->nominal()) +")";
    }

    return expr;
}

QString FreeForm::expression(){
    QString expr = m_gain->expression() + "*(" + m_numeratorExpr + ")/(" + m_denominatorExpr + ")";

    if (m_delay->isUncertain()){
        expr += " * e^(-s*" + m_delay->name() + ")";
    }else if (m_delay->nominal() != 0){
        expr += " * e^(-s*" + QString::number(m_delay->nominal()) +")";
    }

    return expr;
}

LtiSystem::SystemType FreeForm::type(){
    return SystemType::FreeForm;
}

LtiSystem * FreeForm::create(QString name, QVector<Parameter *> *numerator, QVector<Parameter *> *denominator,
                               Parameter *k, Parameter *delay, QString numeratorExpr, QString denominatorExpr){

    //An unspecified delay means a zero delay.
    return new FreeForm (name, numerator, denominator, k,
                             delay == NULL ? new Parameter(0.0) : delay, numeratorExpr, denominatorExpr);
}


QString FreeForm::numeratorString(){
    return m_numeratorExpr;
}

QString FreeForm::denominatorString(){
    return m_denominatorExpr;
}

LtiSystem * FreeForm::clone(){

    Parameter * k = m_gain->clone();
    Parameter * delay = m_delay->clone();

    return this->create(this->name(), Parameter::cloneVector(m_numerator),
                        Parameter::cloneVector(m_denominator), k, delay,
                        m_numeratorExpr, m_denominatorExpr);
}

} // namespace qftbx
