#include <vector>
#include <cstdint>
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

QString TimeConstantGain::expression (std::vector <double> * numerator, std::vector <double> * denominator,
                             double k, double delay, double omega){
    std::int32_t sizeDen = denominator->size();
    std::int32_t sizeNum = numerator->size();

    QString expr;

    expr += QString::number(k) + "*(";


    for (std::int32_t i = 0; i < sizeNum-1; i++){

        expr += "((("+ QString::number(omega) + "*i) /" + QString::number(numerator->at(i)) + ")+1) *";
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    } else{
        expr += "(((" + QString::number(omega) + "*i) / " + QString::number(numerator->back()) + ")+1)) / (";
    }


    for (std::int32_t i = 0; i < sizeDen-1; i++){

        expr += "((("+ QString::number(omega) + "*i) / " + QString::number(denominator->at(i)) + ")+1) *";
    }

    if (sizeDen == 0){
        expr += "(1))";
    }else {
        expr += "((("+ QString::number(omega) + "*i) /" + QString::number(denominator->back()) + ")+1))";
    }


    if (delay != 0){
        expr += "* e^(-i*" + QString::number(omega) + "*" + QString::number(delay) +")";
    }


    return expr;
}

QString TimeConstantGain::expression(double w){

    std::int32_t sizeDen = m_denominator.size();
    std::int32_t sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += QString::number(m_gain.nominal()) + "*(";
    }


    for (std::int32_t i = 0; i < sizeNum-1; i++){

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

    for (std::int32_t i = 0; i < sizeDen-1; i++){

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
    std::int32_t sizeDen = m_denominator.size();
    std::int32_t sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += QString::number(m_gain.nominal()) + "*(";
    }


    for (std::int32_t i = 0; i < sizeNum-1; i++){

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

    for (std::int32_t i = 0; i < sizeDen-1; i++){

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



std::complex <double> TimeConstantGain::evaluateNumerator(std::vector <double> * nume, double omega){


    if (nume->empty()){
        return std::complex <double> (1);
    }

    std::int32_t sizeNum = nume->size();
    QString expr = "(";


    for (std::int32_t i = 0; i < sizeNum-1; i++){

        expr += "((("+ QString::number(omega) + "*i) /" + QString::number(nume->at(i)) + ")+1) *";
    }

    expr += "(((" + QString::number(omega) + "*i) / " + QString::number(nume->back()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

std::complex <double> TimeConstantGain::evaluateDenominator(std::vector <double> * deno, double omega){

    if (deno->empty()){
        return std::complex <double> (1);
    }

    std::int32_t sizeDen = deno->size();
    QString expr = "(";

    for (std::int32_t i = 0; i < sizeDen-1; i++){

        expr += "((("+ QString::number(omega) + "*i) / " + QString::number(deno->at(i)) + ")+1) *";
    }

    expr += "((("+ QString::number(omega) + "*i) /" + QString::number(deno->back()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

} // namespace qftbx

//P(s) = k * prod(s/tau_n[i] + 1) / prod(s/tau_d[i] + 1) at s = j*w, times
//the pure delay. An empty list is the constant 1, as the expression
//generator writes it. A zero time constant divides by zero here exactly as
//it did in the text.
std::complex <double> TimeConstantGain::valueAt(double w, const std::vector<double> & numerator,
                                               const std::vector<double> & denominator,
                                               double gain, double delay)
{
    const std::complex<double> s(0.0, w);

    std::complex<double> num(1.0, 0.0);
    for (const double constant : numerator) {
        num *= s / constant + 1.0;
    }

    std::complex<double> den(1.0, 0.0);
    for (const double constant : denominator) {
        den *= s / constant + 1.0;
    }

    return gain * num / den * std::exp(-s * delay);
}
