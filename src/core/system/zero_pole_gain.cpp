#include "zero_pole_gain.h"

using namespace std;
using namespace mup;

namespace qftbx {

ZeroPoleGain::ZeroPoleGain(QString name, QVector<Parameter *> *numerator, QVector<Parameter *> *denominator, Parameter *k, Parameter *delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
}

ZeroPoleGain::~ZeroPoleGain(){
}

LtiSystem * ZeroPoleGain::create (QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator,
                             Parameter * k, Parameter* delay, QString numeratorExpr __attribute__((unused)), QString denominatorExpr __attribute__((unused))){
    //An unspecified delay means a zero delay.
    return new ZeroPoleGain(name, numerator, denominator, k, delay == NULL ? new Parameter(0.0) : delay);
}



QString ZeroPoleGain::expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                            qreal k, qreal delay, qreal omega){
    qint32 sizeDen = denominator->size();
    qint32 sizeNum = numerator->size();

    QString expr;


    expr += QString::number(k) + "*(";


    if (numerator->isEmpty()){
        expr += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            expr += "(("+ QString::number(omega) + "*i) +" + QString::number(numerator->at(i)) + ") *";
        }

        expr += "((" + QString::number(omega) + "*i) + " + QString::number(numerator->last()) + ")) / (";
    }


    if (denominator->isEmpty()){
        expr += "1)";
    } else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            expr += "(("+ QString::number(omega) + "*i) + " + QString::number(denominator->at(i)) + ") *";
        }

        expr += "(("+ QString::number(omega) + "*i) + " + QString::number(denominator->last()) + "))";
    }

    if (delay != 0){
        expr += "* e^(-i*" + QString::number(omega) + "*" + QString::number(delay) +")";
    }


    return expr;
}

QString ZeroPoleGain::expression(qreal w){

    qint32 sizeDen = m_denominator->size();
    qint32 sizeNum = m_numerator->size();

    QString expr;

    if (m_gain->isUncertain()){
        expr += m_gain->name() + "*(";
    }else {
        expr += QString::number(m_gain->nominal()) + "*(";
    }

    if (m_numerator->isEmpty()){
        expr += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            if (m_numerator->at(i)->isUncertain()){
                expr += "((" + QString::number(w) + "*i) + " + m_numerator->at(i)->name() + ") *";
            } else {
                expr += "(("+ QString::number(w) + "*i) +" + QString::number(m_numerator->at(i)->nominal()) + ") *";
            }
        }

        if(m_numerator->last()->isUncertain()){
            expr += "((" + QString::number(w) + "*i) + " + m_numerator->last()->name() + ")) / (";
        } else {
            expr += "((" + QString::number(w) + "*i) + " + QString::number(m_numerator->last()->nominal()) + ")) / (";
        }
    }


    if (m_denominator->isEmpty()){
        expr += "1)";
    } else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            if (m_denominator->at(i)->isUncertain()){
                expr += "((" + QString::number(w) + "*i) + " + m_denominator->at(i)->name() + ") *";
            } else {
                expr += "(("+ QString::number(w) + "*i) + " + QString::number(m_denominator->at(i)->nominal()) + ") *";
            }
        }


        if (m_denominator->last()->isUncertain()){
            expr += "((" + QString::number(w) + "*i) + " + m_denominator->last()->name() + "))";
        }else{
            expr += "(("+ QString::number(w) + "*i) + " + QString::number(m_denominator->last()->nominal()) + "))";
        }
    }


    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal, so the template sweep can drive
    //it) or a non-zero constant.
    if (m_delay->isUncertain()){
        expr += "* e^(-i*" + QString::number(w) + "*" + m_delay->name() + ")";
    }else if (m_delay->nominal() != 0){
        expr += "* e^(-i*" + QString::number(w) + "*" + QString::number(m_delay->nominal()) +")";
    }

    return expr;
}


LtiSystem::SystemType ZeroPoleGain::type(){
    return SystemType::ZeroPoleGain;
}


QString ZeroPoleGain::expression(){
    qint32 sizeDen = m_denominator->size();
    qint32 sizeNum = m_numerator->size();

    QString expr;

    if (m_gain->isUncertain()){
        expr += m_gain->name() + "*(";
    }else {
        expr += QString::number(m_gain->nominal()) + "*(";
    }

    if (m_numerator->isEmpty()){
        expr += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            if (m_numerator->at(i)->isUncertain()){
                expr += "(s + " + m_numerator->at(i)->name() + ") *";
            } else {
                expr += "(s +" + QString::number(m_numerator->at(i)->nominal()) + ") *";
            }
        }

        if(m_numerator->last()->isUncertain()){
            expr += "(s + " + m_numerator->last()->name() + ")) / (";
        } else {
            expr += "(s + " + QString::number(m_numerator->last()->nominal()) + ")) / (";
        }
    }


    if (m_denominator->isEmpty()){
        expr += "1)";
    }else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            if (m_denominator->at(i)->isUncertain()){
                expr += "(s + " + m_denominator->at(i)->name() + ") *";
            } else {
                expr += "(s + " + QString::number(m_denominator->at(i)->nominal()) + ") *";
            }
        }

        if (m_denominator->last()->isUncertain()){
            expr += "(s + " + m_denominator->last()->name() + "))";
        }else{
            expr += "(s + " + QString::number(m_denominator->last()->nominal()) + "))";
        }
    }

    if (m_delay->isUncertain()){
        expr += " * e^(-s*" + m_delay->name() + ")";
    }else if (m_delay->nominal() != 0){
        expr += " * e^(-s*" + QString::number(m_delay->nominal()) +")";
    }

    return expr;
}


std::complex <qreal> ZeroPoleGain::evaluateNumerator(QVector <qreal> * nume, qreal omega){

    if (nume->isEmpty()){
        return std::complex <qreal>(1);
    }

    qint32 sizeNum = nume->size();
    QString expr = "(";

    for (qint32 i = 0; i < sizeNum-1; i++){

        expr += "(("+ QString::number(omega) + "*i) +" + QString::number(nume->at(i)) + ") *";
    }

    expr += "((" + QString::number(omega) + "*i) + " + QString::number(nume->last()) + "))";


    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> ZeroPoleGain::evaluateDenominator(QVector <qreal> * deno, qreal omega){

    if (deno->isEmpty()){
        return std::complex <qreal>(1);
    }

    qint32 sizeDen = deno->size();
    QString expr = "(";

    for (qint32 i = 0; i < sizeDen-1; i++){

        expr += "(("+ QString::number(omega) + "*i) + " + QString::number(deno->at(i)) + ") *";
    }

    expr += "(("+ QString::number(omega) + "*i) + " + QString::number(deno->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

} // namespace qftbx
