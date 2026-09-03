#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <complex>

#include "src/core/text_tokens.h"
#include "time_constant_gain.h"

using namespace std;

namespace qftbx {

TimeConstantGain::TimeConstantGain(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator,k,delay)
{
}

TimeConstantGain::~TimeConstantGain(){
}

std::unique_ptr<LtiSystem> TimeConstantGain::create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay, std::string numeratorExpr __attribute__((unused)), std::string denominatorExpr __attribute__((unused))){
    return std::make_unique<TimeConstantGain>(name, std::move(numerator), std::move(denominator),
                                              std::move(k), std::move(delay));
}

std::string TimeConstantGain::expression (std::vector <double> * numerator, std::vector <double> * denominator,
                             double k, double delay, double omega){
    std::size_t sizeDen = denominator->size();
    std::size_t sizeNum = numerator->size();

    std::string expr;

    expr += qftbx::text::number(k) + "*(";


    for (std::size_t i = 0; i + 1 < sizeNum; i++){

        expr += "((("+ qftbx::text::number(omega) + "*i) /" + qftbx::text::number(numerator->at(i)) + ")+1) *";
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    } else{
        expr += "(((" + qftbx::text::number(omega) + "*i) / " + qftbx::text::number(numerator->back()) + ")+1)) / (";
    }


    for (std::size_t i = 0; i + 1 < sizeDen; i++){

        expr += "((("+ qftbx::text::number(omega) + "*i) / " + qftbx::text::number(denominator->at(i)) + ")+1) *";
    }

    if (sizeDen == 0){
        expr += "(1))";
    }else {
        expr += "((("+ qftbx::text::number(omega) + "*i) /" + qftbx::text::number(denominator->back()) + ")+1))";
    }


    if (delay != 0){
        expr += "* e^(-i*" + qftbx::text::number(omega) + "*" + qftbx::text::number(delay) +")";
    }


    return expr;
}

std::string TimeConstantGain::expression(double w){

    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += qftbx::text::number(m_gain.nominal()) + "*(";
    }


    for (std::size_t i = 0; i + 1 < sizeNum; i++){

        if (m_numerator[i].isUncertain()){
            expr += "(((" + qftbx::text::number(w) + "*i) / " + m_numerator[i].name() + ")+1) *";
        } else {
            expr += "((("+ qftbx::text::number(w) + "*i) /" + qftbx::text::number(m_numerator[i].nominal()) + ")+1) *";
        }
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    }else {
        if(m_numerator.back().isUncertain()){
            expr += "(((" + qftbx::text::number(w) + "*i) / " + m_numerator.back().name() + ")+1)) / (";
        } else {
            expr += "(((" + qftbx::text::number(w) + "*i) / " + qftbx::text::number(m_numerator.back().nominal()) + ")+1)) / (";
        }
    }

    for (std::size_t i = 0; i + 1 < sizeDen; i++){

        if (m_denominator[i].isUncertain()){
            expr += "(((" + qftbx::text::number(w) + "*i) / " + m_denominator[i].name() + ")+1) *";
        } else {
            expr += "((("+ qftbx::text::number(w) + "*i) / " + qftbx::text::number(m_denominator[i].nominal()) + ")+1) *";
        }
    }

    if (sizeDen == 0){
        expr += "(1))";
    } else {

        if (m_denominator.back().isUncertain()){
            expr += "(((" + qftbx::text::number(w) + "*i) / " + m_denominator.back().name() + ")+1))";
        }else{
            expr += "((("+ qftbx::text::number(w) + "*i) /" + qftbx::text::number(m_denominator.back().nominal()) + ")+1))";
        }
    }

    //A pure delay is e^(-s*tau) => e^(-i*w*tau). Emitted when the delay is
    //uncertain (even with a zero nominal) or a non-zero constant, using the
    //parameter's real name.
    if (m_delay.isUncertain()){
        expr += "* e^(-i*" + qftbx::text::number(w) + "*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-i*" + qftbx::text::number(w) + "*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}

std::string TimeConstantGain::expression(){
    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += m_gain.name() + "*(";
    }else {
        expr += qftbx::text::number(m_gain.nominal()) + "*(";
    }


    for (std::size_t i = 0; i + 1 < sizeNum; i++){

        if (m_numerator[i].isUncertain()){
            expr += "(s / " + m_numerator[i].name() + "+1) *";
        } else {
            expr += "(s /" + qftbx::text::number(m_numerator[i].nominal()) + "+1) *";
        }
    }

    if (sizeNum == 0){
        expr += "(1)) / (";
    }else {

        if(m_numerator.back().isUncertain()){
            expr += "(s / " + m_numerator.back().name() + "+1)) / (";
        } else {
            expr += "(s / " + qftbx::text::number(m_numerator.back().nominal()) + "+1)) / (";
        }
    }

    for (std::size_t i = 0; i + 1 < sizeDen; i++){

        if (m_denominator[i].isUncertain()){
            expr += "(s / " + m_denominator[i].name() + "+1) *";
        } else {
            expr += "(s / " + qftbx::text::number(m_denominator[i].nominal()) + "+1) *";
        }
    }

    if (sizeDen == 0){
        expr += "(1))";
    } else {

        if (m_denominator.back().isUncertain()){
            expr += "(s / " + m_denominator.back().name() + "+1))";
        }else{
            expr += "(s /" + qftbx::text::number(m_denominator.back().nominal()) + "+1))";
        }
    }

    if (m_delay.isUncertain()){
        expr += "* e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += "* e^(-s*" + qftbx::text::number(m_delay.nominal()) +")";
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

    std::size_t sizeNum = nume->size();
    std::string expr = "(";


    for (std::size_t i = 0; i + 1 < sizeNum; i++){

        expr += "((("+ qftbx::text::number(omega) + "*i) /" + qftbx::text::number(nume->at(i)) + ")+1) *";
    }

    expr += "(((" + qftbx::text::number(omega) + "*i) / " + qftbx::text::number(nume->back()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr);

    return p.Eval().GetComplex();
}

std::complex <double> TimeConstantGain::evaluateDenominator(std::vector <double> * deno, double omega){

    if (deno->empty()){
        return std::complex <double> (1);
    }

    std::size_t sizeDen = deno->size();
    std::string expr = "(";

    for (std::size_t i = 0; i + 1 < sizeDen; i++){

        expr += "((("+ qftbx::text::number(omega) + "*i) / " + qftbx::text::number(deno->at(i)) + ")+1) *";
    }

    expr += "((("+ qftbx::text::number(omega) + "*i) /" + qftbx::text::number(deno->back()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr);

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
