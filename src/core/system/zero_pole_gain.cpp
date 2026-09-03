#include <vector>
#include <cstdint>
#include <cmath>
#include <complex>

#include "zero_pole_gain.h"

using namespace std;
using namespace mup;

namespace qftbx {

ZeroPoleGain::ZeroPoleGain(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
}

ZeroPoleGain::~ZeroPoleGain(){
}

std::unique_ptr<LtiSystem> ZeroPoleGain::create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                             Parameter k, Parameter delay, QString numeratorExpr __attribute__((unused)), QString denominatorExpr __attribute__((unused))){
    return std::make_unique<ZeroPoleGain>(name, std::move(numerator), std::move(denominator),
                                          std::move(k), std::move(delay));
}



QString ZeroPoleGain::expression (std::vector <double> * numerator, std::vector <double> * denominator,
                            double k, double delay, double omega){
    std::int32_t sizeDen = denominator->size();
    std::int32_t sizeNum = numerator->size();

    QString expr;


    expr += QString::number(k) + "*(";


    if (numerator->empty()){
        expr += "1) / (";
    } else {
        for (std::int32_t i = 0; i < sizeNum-1; i++){

            expr += "(("+ QString::number(omega) + "*i) +" + QString::number(numerator->at(i)) + ") *";
        }

        expr += "((" + QString::number(omega) + "*i) + " + QString::number(numerator->back()) + ")) / (";
    }


    if (denominator->empty()){
        expr += "1)";
    } else {
        for (std::int32_t i = 0; i < sizeDen-1; i++){

            expr += "(("+ QString::number(omega) + "*i) + " + QString::number(denominator->at(i)) + ") *";
        }

        expr += "(("+ QString::number(omega) + "*i) + " + QString::number(denominator->back()) + "))";
    }

    if (delay != 0){
        expr += "* e^(-i*" + QString::number(omega) + "*" + QString::number(delay) +")";
    }


    return expr;
}

QString ZeroPoleGain::expression(double w){

    std::int32_t sizeDen = m_denominator.size();
    std::int32_t sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += QString::number(m_gain.nominal()) + "*(";
    }

    if (m_numerator.empty()){
        expr += "1) / (";
    } else {
        for (std::int32_t i = 0; i < sizeNum-1; i++){

            if (m_numerator[i].isUncertain()){
                expr += "((" + QString::number(w) + "*i) + " + m_numerator[i].name() + ") *";
            } else {
                expr += "(("+ QString::number(w) + "*i) +" + QString::number(m_numerator[i].nominal()) + ") *";
            }
        }

        if(m_numerator.back().isUncertain()){
            expr += "((" + QString::number(w) + "*i) + " + m_numerator.back().name() + ")) / (";
        } else {
            expr += "((" + QString::number(w) + "*i) + " + QString::number(m_numerator.back().nominal()) + ")) / (";
        }
    }


    if (m_denominator.empty()){
        expr += "1)";
    } else {
        for (std::int32_t i = 0; i < sizeDen-1; i++){

            if (m_denominator[i].isUncertain()){
                expr += "((" + QString::number(w) + "*i) + " + m_denominator[i].name() + ") *";
            } else {
                expr += "(("+ QString::number(w) + "*i) + " + QString::number(m_denominator[i].nominal()) + ") *";
            }
        }


        if (m_denominator.back().isUncertain()){
            expr += "((" + QString::number(w) + "*i) + " + m_denominator.back().name() + "))";
        }else{
            expr += "(("+ QString::number(w) + "*i) + " + QString::number(m_denominator.back().nominal()) + "))";
        }
    }


    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal, so the template sweep can drive
    //it) or a non-zero constant.
    if (m_delay.isUncertain()){
        expr += "* e^(-i*" + QString::number(w) + "*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-i*" + QString::number(w) + "*" + QString::number(m_delay.nominal()) +")";
    }

    return expr;
}


LtiSystem::SystemType ZeroPoleGain::type(){
    return SystemType::ZeroPoleGain;
}


QString ZeroPoleGain::expression(){
    std::int32_t sizeDen = m_denominator.size();
    std::int32_t sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += QString::number(m_gain.nominal()) + "*(";
    }

    if (m_numerator.empty()){
        expr += "1) / (";
    } else {
        for (std::int32_t i = 0; i < sizeNum-1; i++){

            if (m_numerator[i].isUncertain()){
                expr += "(s + " + m_numerator[i].name() + ") *";
            } else {
                expr += "(s +" + QString::number(m_numerator[i].nominal()) + ") *";
            }
        }

        if(m_numerator.back().isUncertain()){
            expr += "(s + " + m_numerator.back().name() + ")) / (";
        } else {
            expr += "(s + " + QString::number(m_numerator.back().nominal()) + ")) / (";
        }
    }


    if (m_denominator.empty()){
        expr += "1)";
    }else {
        for (std::int32_t i = 0; i < sizeDen-1; i++){

            if (m_denominator[i].isUncertain()){
                expr += "(s + " + m_denominator[i].name() + ") *";
            } else {
                expr += "(s + " + QString::number(m_denominator[i].nominal()) + ") *";
            }
        }

        if (m_denominator.back().isUncertain()){
            expr += "(s + " + m_denominator.back().name() + "))";
        }else{
            expr += "(s + " + QString::number(m_denominator.back().nominal()) + "))";
        }
    }

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + QString::number(m_delay.nominal()) +")";
    }

    return expr;
}


std::complex <double> ZeroPoleGain::evaluateNumerator(std::vector <double> * nume, double omega){

    if (nume->empty()){
        return std::complex <double>(1);
    }

    std::int32_t sizeNum = nume->size();
    QString expr = "(";

    for (std::int32_t i = 0; i < sizeNum-1; i++){

        expr += "(("+ QString::number(omega) + "*i) +" + QString::number(nume->at(i)) + ") *";
    }

    expr += "((" + QString::number(omega) + "*i) + " + QString::number(nume->back()) + "))";


    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

std::complex <double> ZeroPoleGain::evaluateDenominator(std::vector <double> * deno, double omega){

    if (deno->empty()){
        return std::complex <double>(1);
    }

    std::int32_t sizeDen = deno->size();
    QString expr = "(";

    for (std::int32_t i = 0; i < sizeDen-1; i++){

        expr += "(("+ QString::number(omega) + "*i) + " + QString::number(deno->at(i)) + ") *";
    }

    expr += "(("+ QString::number(omega) + "*i) + " + QString::number(deno->back()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

} // namespace qftbx

//P(s) = k * prod(s + z[i]) / prod(s + p[i]) at s = j*w, times the pure
//delay. An empty list is the constant 1, as the expression generator writes
//it. Note the sign: the stored coefficients are the NEGATED roots, which is
//what the textual form (jw) + z always computed.
std::complex <double> ZeroPoleGain::valueAt(double w, const std::vector<double> & numerator,
                                           const std::vector<double> & denominator,
                                           double gain, double delay)
{
    const std::complex<double> s(0.0, w);

    std::complex<double> num(1.0, 0.0);
    for (const double zero : numerator) {
        num *= s + zero;
    }

    std::complex<double> den(1.0, 0.0);
    for (const double pole : denominator) {
        den *= s + pole;
    }

    return gain * num / den * std::exp(-s * delay);
}
