#include "polynomial_form.h"

using namespace std;

namespace qftbx {

PolynomialForm::PolynomialForm(QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator, Parameter * k, Parameter* delay):
    TransferFunction(name, numerator, denominator, k , delay)
{

}

PolynomialForm::~PolynomialForm(){
}

LtiSystem * PolynomialForm::create (QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator,
                               Parameter * k, Parameter* delay, QString numeratorExpr __attribute__((unused)), QString denominatorExpr __attribute__((unused))){
    //An unspecified delay means a zero delay.
    return new PolynomialForm(name, numerator, denominator, k, delay == NULL ? new Parameter(0.0) : delay);
}

LtiSystem::SystemType PolynomialForm::type(){
    return SystemType::PolynomialForm;
}

QString PolynomialForm::expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                              qreal k, qreal delay, qreal omega){

    qint32 sizeDen = denominator->size();
    qint32 sizeNum = numerator->size();

    QString expr;


    expr +=  "(" +  QString::number(k) + "*(";



    for (qint32 i = 1; i < sizeNum; i++){


        expr += "(" + QString::number(numerator->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeNum - i)+ ") +";

    }


    if (sizeNum > 0){
        expr += "(" + QString::number(numerator->last()) + ")) / (";
    } else {
        expr += "(1))/(";
    }


    for (qint32 i = 1; i < sizeDen; i++){


        expr += "(" + QString::number(denominator->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeDen - i) + ") +";

    }

    if (sizeDen > 0){
        expr += "(" + QString::number(denominator->last()) + ")))";
    }else {
        expr += "(1)))";
    }

    if (delay != 0){

        expr += "* e^(-i*" + QString::number(omega) + "*" + QString::number(delay) +")";
    }


    return expr;
}

QString PolynomialForm::expression(qreal w){

    qint32 sizeDen = m_denominator->size();
    qint32 sizeNum = m_numerator->size();

    QString expr;

    if (m_gain->isUncertain()){
        expr += "(" + m_gain->name() + "*(";
    }else {
        expr += "(" + QString::number(m_gain->nominal()) + "*(";
    }


    for (qint32 i = 1; i < sizeNum; i++){

        if (m_numerator->at(i-1)->isUncertain()){
            expr += "(" + m_numerator->at(i-1)->name() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            expr += "(" + QString::number(m_numerator->at(i-1)->nominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (m_numerator->size() > 0){
        if (m_numerator->last()->isUncertain()){
            expr += "(" + m_numerator->last()->name() + ")) / (";
        }else{
            expr += "(" + QString::number(m_numerator->last()->nominal()) + ")) / (";
        }
    } else {
        expr += "(1)) / (";
    }

    for (qint32 i = 1; i < sizeDen; i++){

        if (m_denominator->at(i-1)->isUncertain()){
            expr += "(" + m_denominator->at(i-1)->name() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            expr += "(" + QString::number(m_denominator->at(i-1)->nominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }


    if (m_denominator->size() > 0){
        if (m_denominator->last()->isUncertain()){
            expr += "(" + m_denominator->last()->name() + ")))";
        }else{
            expr += "(" + QString::number(m_denominator->last()->nominal()) + ")))";
        }
    } else {
        expr += "(1)))";
    }

    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal) or a non-zero constant, using the
    //parameter's real name.
    if (m_delay->isUncertain()){
        expr += "* e^(-i*" + QString::number(w) + "*" + m_delay->name() + ")";
    }else if (m_delay->nominal() != 0){
        expr += "* e^(-i*" + QString::number(w) + "*" + QString::number(m_delay->nominal()) +")";
    }

    return expr;
}

QString PolynomialForm::expression(){
    qint32 sizeDen = m_denominator->size();
    qint32 sizeNum = m_numerator->size();

    QString expr;

    if (m_gain->isUncertain()){
        expr += "(" + m_gain->name() + "*(";
    }else {
        expr +="(" + QString::number(m_gain->nominal()) + "*(";
    }


    for (qint32 i = 1; i < sizeNum; i++){

        if (m_numerator->at(i-1)->isUncertain()){
            expr += "(" + m_numerator->at(i-1)->name() + "*s^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            expr += "(" + QString::number(m_numerator->at(i-1)->nominal()) + "*s^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (m_numerator->size() > 0){
        if (m_numerator->last()->isUncertain()){
            expr += "(" + m_numerator->last()->name() + ")) / (";
        }else{
            expr += "(" + QString::number(m_numerator->last()->nominal()) + ")) / (";
        }
    } else {
        expr += "(1)) / (";
    }

    for (qint32 i = 1; i < sizeDen; i++){

        if (m_denominator->at(i-1)->isUncertain()){
            expr += "(" + m_denominator->at(i-1)->name() + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            expr += "(" + QString::number(m_denominator->at(i-1)->nominal()) + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }

    if (m_denominator->size() > 0){
        if (m_denominator->last()->isUncertain()){
            expr += "(" + m_denominator->last()->name() + ")))";
        }else{
            expr += "(" + QString::number(m_denominator->last()->nominal()) + ")))";
        }
    } else {
        expr += "(1)))";
    }

    if (m_delay->isUncertain()){
        expr += " * e^(-s*" + m_delay->name() + ")";
    }else if (m_delay->nominal() != 0){
        expr += " * e^(-s*" + QString::number(m_delay->nominal()) +")";
    }

    return expr;
}

std::complex <qreal> PolynomialForm::evaluateNumerator(QVector <qreal> * nume, qreal omega){

    if (nume->size() == 0){
        return std::complex <qreal> (1, 0);
    }

    qint32 sizeNum = nume->size();
    QString expr = "(";


    for (qint32 i = 1; i < sizeNum; i++){
        expr += "(" + QString::number(nume->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeNum - i)+ ") +";
    }

    expr += "(" + QString::number(nume->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> PolynomialForm::evaluateDenominator(QVector <qreal> * deno, qreal omega){

    if (deno->size() == 0){
        return std::complex <qreal> (1, 0);
    }

    qint32 sizeDen = deno->size();
    QString expr = "(";


    for (qint32 i = 1; i < sizeDen; i++){
        expr += "(" + QString::number(deno->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeDen - i)+ ") +";
    }

    expr += "(" + QString::number(deno->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

} // namespace qftbx
