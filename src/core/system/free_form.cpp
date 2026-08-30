#include "free_form.h"

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

std::complex <qreal> FreeForm::evaluate (QVector <qreal> * numerator, QVector <qreal> * denominator,
                                             qreal k, qreal delay, qreal omega){ //TODO ver que hacer con esto
    return complex <qreal> ();
}

QString FreeForm::expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                               qreal k, qreal delay, qreal omega){//TODO ver que hacer con esto
    return "";
}

std::complex <qreal> FreeForm::evaluateNumerator(QVector <qreal> * nume, qreal omega){


    return complex <qreal> ();
}

std::complex <qreal> FreeForm::evaluateDenominator(QVector <qreal> * deno, qreal omega){


    return complex <qreal> ();
}

QString FreeForm::expression(qreal w){

    QString n = m_numeratorExpr;
    QString d = m_denominatorExpr;

    QString expr = m_gain->expression() + "*(" + n.replace("s", "(" + QString::number(w) + "*i)") + ")/(" +
            d.replace("s", "(" + QString::number(w) + "*i)") + ")";


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
