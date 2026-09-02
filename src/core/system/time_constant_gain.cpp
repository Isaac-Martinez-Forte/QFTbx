#include <cmath>
#include <complex>

#include "time_constant_gain.h"

using namespace std;

namespace qftbx {

TimeConstantGain::TimeConstantGain(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
}

TimeConstantGain::~TimeConstantGain(){
}

std::unique_ptr<LtiSystem> TimeConstantGain::create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay, QString numeratorExpr __attribute__((unused)), QString denominatorExpr __attribute__((unused))){
    return std::make_unique<TimeConstantGain>(name, std::move(numerator), std::move(denominator),
                                              std::move(k), std::move(delay));
}

QString TimeConstantGain::expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega){
    qint32 sizeDen = denominator->size();
    qint32 sizeNum = numerator->size();

    QString expr;

    expr += QString::number(k) + "*(";


    for (qint32 i = 0; i < sizeNum-1; i++){

        expr += "((("+ QString::number(omega) + "*i) /" + QString::number(numerator->at(i)) + ")+1) *";
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    } else{
        expr += "(((" + QString::number(omega) + "*i) / " + QString::number(numerator->last()) + ")+1)) / (";
    }


    for (qint32 i = 0; i < sizeDen-1; i++){

        expr += "((("+ QString::number(omega) + "*i) / " + QString::number(denominator->at(i)) + ")+1) *";
    }

    if (sizeDen == 0){
        expr += "(1))";
    }else {
        expr += "((("+ QString::number(omega) + "*i) /" + QString::number(denominator->last()) + ")+1))";
    }


    if (delay != 0){
        expr += "* e^(-i*" + QString::number(omega) + "*" + QString::number(delay) +")";
    }


    return expr;
}

QString TimeConstantGain::expression(qreal w){

    qint32 sizeDen = m_denominator.size();
    qint32 sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += QString::number(m_gain.nominal()) + "*(";
    }


    for (qint32 i = 0; i < sizeNum-1; i++){

        if (m_numerator[i].isUncertain()){
            expr += "(((" + QString::number(w) + "*i) / " + m_numerator[i].name() + ")+1) *";
        } else {
            expr += "((("+ QString::number(w) + "*i) /" + QString::number(m_numerator[i].nominal()) + ")+1) *";
        }
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    }else {
        if(m_numerator.back().isUncertain()){
            expr += "(((" + QString::number(w) + "*i) / " + m_numerator.back().name() + ")+1)) / (";
        } else {
            expr += "(((" + QString::number(w) + "*i) / " + QString::number(m_numerator.back().nominal()) + ")+1)) / (";
        }
    }

    for (qint32 i = 0; i < sizeDen-1; i++){

        if (m_denominator[i].isUncertain()){
            expr += "(((" + QString::number(w) + "*i) / " + m_denominator[i].name() + ")+1) *";
        } else {
            expr += "((("+ QString::number(w) + "*i) / " + QString::number(m_denominator[i].nominal()) + ")+1) *";
        }
    }

    if (sizeDen == 0){
        expr += "(1))";
    } else {

        if (m_denominator.back().isUncertain()){
            expr += "(((" + QString::number(w) + "*i) / " + m_denominator.back().name() + ")+1))";
        }else{
            expr += "((("+ QString::number(w) + "*i) /" + QString::number(m_denominator.back().nominal()) + ")+1))";
        }
    }

    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal) or a non-zero constant, using the
    //parameter's real name.
    if (m_delay.isUncertain()){
        expr += "* e^(-i*" + QString::number(w) + "*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-i*" + QString::number(w) + "*" + QString::number(m_delay.nominal()) +")";
    }

    return expr;
}

QString TimeConstantGain::expression(){
    qint32 sizeDen = m_denominator.size();
    qint32 sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += QString::number(m_gain.nominal()) + "*(";
    }


    for (qint32 i = 0; i < sizeNum-1; i++){

        if (m_numerator[i].isUncertain()){
            expr += "(s / " + m_numerator[i].name() + "+1) *";
        } else {
            expr += "(s /" + QString::number(m_numerator[i].nominal()) + "+1) *";
        }
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    }else {

        if(m_numerator.back().isUncertain()){
            expr += "(s / " + m_numerator.back().name() + "+1)) / (";
        } else {
            expr += "(s / " + QString::number(m_numerator.back().nominal()) + "+1)) / (";
        }
    }

    for (qint32 i = 0; i < sizeDen-1; i++){

        if (m_denominator[i].isUncertain()){
            expr += "(s / " + m_denominator[i].name() + "+1) *";
        } else {
            expr += "(s / " + QString::number(m_denominator[i].nominal()) + "+1) *";
        }
    }

    if (sizeDen == 0){
        expr += "(1))";
    } else {

        if (m_denominator.back().isUncertain()){
            expr += "(s / " + m_denominator.back().name() + "+1))";
        }else{
            expr += "(s /" + QString::number(m_denominator.back().nominal()) + "+1))";
        }
    }

    if (m_delay.isUncertain()){
        expr += "* e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-s*" + QString::number(m_delay.nominal()) +")";
    }

    return expr;
}

LtiSystem::SystemType TimeConstantGain::type(){
    return SystemType::TimeConstantGain;
}



std::complex <qreal> TimeConstantGain::evaluateNumerator(QVector <qreal> * nume, qreal omega){


    if (nume->isEmpty()){
        return std::complex <qreal> (1);
    }

    qint32 sizeNum = nume->size();
    QString expr = "(";


    for (qint32 i = 0; i < sizeNum-1; i++){

        expr += "((("+ QString::number(omega) + "*i) /" + QString::number(nume->at(i)) + ")+1) *";
    }

    expr += "(((" + QString::number(omega) + "*i) / " + QString::number(nume->last()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> TimeConstantGain::evaluateDenominator(QVector <qreal> * deno, qreal omega){

    if (deno->isEmpty()){
        return std::complex <qreal> (1);
    }

    qint32 sizeDen = deno->size();
    QString expr = "(";

    for (qint32 i = 0; i < sizeDen-1; i++){

        expr += "((("+ QString::number(omega) + "*i) / " + QString::number(deno->at(i)) + ")+1) *";
    }

    expr += "((("+ QString::number(omega) + "*i) /" + QString::number(deno->last()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

} // namespace qftbx

//P(s) = k * prod(s/tau_n[i] + 1) / prod(s/tau_d[i] + 1) at s = j*w, times
//the pure delay. An empty list is the constant 1, as the expression
//generator writes it. A zero time constant divides by zero here exactly as
//it did in the text.
std::complex <qreal> TimeConstantGain::valueAt(qreal w, const std::vector<qreal> & numerator,
                                               const std::vector<qreal> & denominator,
                                               qreal gain, qreal delay)
{
    const std::complex<qreal> s(0.0, w);

    std::complex<qreal> num(1.0, 0.0);
    for (const qreal constant : numerator) {
        num *= s / constant + 1.0;
    }

    std::complex<qreal> den(1.0, 0.0);
    for (const qreal constant : denominator) {
        den *= s / constant + 1.0;
    }

    return gain * num / den * std::exp(-s * delay);
}
