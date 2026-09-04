#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <complex>

#include "src/core/text_tokens.h"
#include "zero_pole_gain.h"

#include "mpParser.h"

namespace qftbx {

ZeroPoleGain::ZeroPoleGain(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
}

std::unique_ptr<LtiSystem> ZeroPoleGain::create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                             Parameter k, Parameter delay, [[maybe_unused]] std::string numeratorExpr, [[maybe_unused]] std::string denominatorExpr){
    return std::make_unique<ZeroPoleGain>(name, std::move(numerator), std::move(denominator),
                                          std::move(k), std::move(delay));
}



std::string ZeroPoleGain::expression (std::vector <double> * numerator, std::vector <double> * denominator,
                            double k, double delay, double omega){
    std::size_t sizeDen = denominator->size();
    std::size_t sizeNum = numerator->size();

    std::string expr;


    expr += qftbx::text::number(k) + "*(";


    if (numerator->empty()){
        expr += "1) / (";
    } else {
        for (std::size_t i = 0; i + 1 < sizeNum; i++){

            expr += "(("+ qftbx::text::number(omega) + "*i) +" + qftbx::text::number(numerator->at(i)) + ") *";
        }

        expr += "((" + qftbx::text::number(omega) + "*i) + " + qftbx::text::number(numerator->back()) + ")) / (";
    }


    if (denominator->empty()){
        expr += "1)";
    } else {
        for (std::size_t i = 0; i + 1 < sizeDen; i++){

            expr += "(("+ qftbx::text::number(omega) + "*i) + " + qftbx::text::number(denominator->at(i)) + ") *";
        }

        expr += "(("+ qftbx::text::number(omega) + "*i) + " + qftbx::text::number(denominator->back()) + "))";
    }

    if (delay != 0){
        expr += "* e^(-i*" + qftbx::text::number(omega) + "*" + qftbx::text::number(delay) +")";
    }


    return expr;
}

std::string ZeroPoleGain::expression(double w){

    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += qftbx::text::number(m_gain.nominal()) + "*(";
    }

    if (m_numerator.empty()){
        expr += "1) / (";
    } else {
        for (std::size_t i = 0; i + 1 < sizeNum; i++){

            if (m_numerator[i].isUncertain()){
                expr += "((" + qftbx::text::number(w) + "*i) + " + m_numerator[i].name() + ") *";
            } else {
                expr += "(("+ qftbx::text::number(w) + "*i) +" + qftbx::text::number(m_numerator[i].nominal()) + ") *";
            }
        }

        if(m_numerator.back().isUncertain()){
            expr += "((" + qftbx::text::number(w) + "*i) + " + m_numerator.back().name() + ")) / (";
        } else {
            expr += "((" + qftbx::text::number(w) + "*i) + " + qftbx::text::number(m_numerator.back().nominal()) + ")) / (";
        }
    }


    if (m_denominator.empty()){
        expr += "1)";
    } else {
        for (std::size_t i = 0; i + 1 < sizeDen; i++){

            if (m_denominator[i].isUncertain()){
                expr += "((" + qftbx::text::number(w) + "*i) + " + m_denominator[i].name() + ") *";
            } else {
                expr += "(("+ qftbx::text::number(w) + "*i) + " + qftbx::text::number(m_denominator[i].nominal()) + ") *";
            }
        }


        if (m_denominator.back().isUncertain()){
            expr += "((" + qftbx::text::number(w) + "*i) + " + m_denominator.back().name() + "))";
        }else{
            expr += "(("+ qftbx::text::number(w) + "*i) + " + qftbx::text::number(m_denominator.back().nominal()) + "))";
        }
    }


    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal, so the template sweep can drive
    //it) or a non-zero constant.
    if (m_delay.isUncertain()){
        expr += "* e^(-i*" + qftbx::text::number(w) + "*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-i*" + qftbx::text::number(w) + "*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}


LtiSystem::SystemType ZeroPoleGain::type(){
    return SystemType::ZeroPoleGain;
}


std::string ZeroPoleGain::expression(){
    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += qftbx::text::number(m_gain.nominal()) + "*(";
    }

    if (m_numerator.empty()){
        expr += "1) / (";
    } else {
        for (std::size_t i = 0; i + 1 < sizeNum; i++){

            if (m_numerator[i].isUncertain()){
                expr += "(s + " + m_numerator[i].name() + ") *";
            } else {
                expr += "(s +" + qftbx::text::number(m_numerator[i].nominal()) + ") *";
            }
        }

        if(m_numerator.back().isUncertain()){
            expr += "(s + " + m_numerator.back().name() + ")) / (";
        } else {
            expr += "(s + " + qftbx::text::number(m_numerator.back().nominal()) + ")) / (";
        }
    }


    if (m_denominator.empty()){
        expr += "1)";
    }else {
        for (std::size_t i = 0; i + 1 < sizeDen; i++){

            if (m_denominator[i].isUncertain()){
                expr += "(s + " + m_denominator[i].name() + ") *";
            } else {
                expr += "(s + " + qftbx::text::number(m_denominator[i].nominal()) + ") *";
            }
        }

        if (m_denominator.back().isUncertain()){
            expr += "(s + " + m_denominator.back().name() + "))";
        }else{
            expr += "(s + " + qftbx::text::number(m_denominator.back().nominal()) + "))";
        }
    }

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}


std::complex <double> ZeroPoleGain::evaluateNumerator(std::vector <double> * nume, double omega){

    if (nume->empty()){
        return std::complex <double>(1);
    }

    std::size_t sizeNum = nume->size();
    std::string expr = "(";

    for (std::size_t i = 0; i + 1 < sizeNum; i++){

        expr += "(("+ qftbx::text::number(omega) + "*i) +" + qftbx::text::number(nume->at(i)) + ") *";
    }

    expr += "((" + qftbx::text::number(omega) + "*i) + " + qftbx::text::number(nume->back()) + "))";


    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr);

    return p.Eval().GetComplex();
}

std::complex <double> ZeroPoleGain::evaluateDenominator(std::vector <double> * deno, double omega){

    if (deno->empty()){
        return std::complex <double>(1);
    }

    std::size_t sizeDen = deno->size();
    std::string expr = "(";

    for (std::size_t i = 0; i + 1 < sizeDen; i++){

        expr += "(("+ qftbx::text::number(omega) + "*i) + " + qftbx::text::number(deno->at(i)) + ") *";
    }

    expr += "(("+ qftbx::text::number(omega) + "*i) + " + qftbx::text::number(deno->back()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr);

    return p.Eval().GetComplex();
}


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

} // namespace qftbx
