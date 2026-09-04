#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <complex>

#include "src/core/text_tokens.h"
#include "polynomial_form.h"

#include "mpParser.h"

namespace qftbx {

PolynomialForm::PolynomialForm(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator, k , delay)
{

}

std::unique_ptr<LtiSystem> PolynomialForm::create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                               Parameter k, Parameter delay, [[maybe_unused]] std::string numeratorExpr, [[maybe_unused]] std::string denominatorExpr){
    return std::make_unique<PolynomialForm>(name, std::move(numerator), std::move(denominator),
                                            std::move(k), std::move(delay));
}

LtiSystem::SystemType PolynomialForm::type(){
    return SystemType::PolynomialForm;
}

std::string PolynomialForm::expression (std::vector <double> * numerator, std::vector <double> * denominator,
                              double k, double delay, double omega){

    std::size_t sizeDen = denominator->size();
    std::size_t sizeNum = numerator->size();

    std::string expr;


    expr +=  "(" +  qftbx::text::number(k) + "*(";



    for (std::size_t i = 1; i < sizeNum; i++){


        expr += "(" + qftbx::text::number(numerator->at(i-1)) + "*(" + qftbx::text::number(omega) + "*i)^" +
                std::to_string(sizeNum - i)+ ") +";

    }


    if (sizeNum > 0){
        expr += "(" + qftbx::text::number(numerator->back()) + ")) / (";
    } else {
        expr += "(1))/(";
    }


    for (std::size_t i = 1; i < sizeDen; i++){


        expr += "(" + qftbx::text::number(denominator->at(i-1)) + "*(" + qftbx::text::number(omega) + "*i)^" +
                std::to_string(sizeDen - i) + ") +";

    }

    if (sizeDen > 0){
        expr += "(" + qftbx::text::number(denominator->back()) + ")))";
    }else {
        expr += "(1)))";
    }

    if (delay != 0){

        expr += "* e^(-i*" + qftbx::text::number(omega) + "*" + qftbx::text::number(delay) +")";
    }


    return expr;
}

std::string PolynomialForm::expression(double w){

    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += "(" + m_gain.name() + "*(";
    }else {
        expr += "(" + qftbx::text::number(m_gain.nominal()) + "*(";
    }


    for (std::size_t i = 1; i < sizeNum; i++){

        if (m_numerator[i-1].isUncertain()){
            expr += "(" + m_numerator[i-1].name() + "*(" + qftbx::text::number(w) + "*i)^" +
                    std::to_string(sizeNum - i) + ") +";
        } else {
            expr += "(" + qftbx::text::number(m_numerator[i-1].nominal()) + "*(" + qftbx::text::number(w) + "*i)^" +
                    std::to_string(sizeNum - i)+ ") +";
        }
    }

    if (m_numerator.size() > 0){
        if (m_numerator.back().isUncertain()){
            expr += "(" + m_numerator.back().name() + ")) / (";
        }else{
            expr += "(" + qftbx::text::number(m_numerator.back().nominal()) + ")) / (";
        }
    } else {
        expr += "(1)) / (";
    }

    for (std::size_t i = 1; i < sizeDen; i++){

        if (m_denominator[i-1].isUncertain()){
            expr += "(" + m_denominator[i-1].name() + "*(" + qftbx::text::number(w) + "*i)^" +
                    std::to_string(sizeDen - i) + ") +";
        } else {
            expr += "(" + qftbx::text::number(m_denominator[i-1].nominal()) + "*(" + qftbx::text::number(w) + "*i)^" +
                    std::to_string(sizeDen - i) + ") +";
        }
    }


    if (m_denominator.size() > 0){
        if (m_denominator.back().isUncertain()){
            expr += "(" + m_denominator.back().name() + ")))";
        }else{
            expr += "(" + qftbx::text::number(m_denominator.back().nominal()) + ")))";
        }
    } else {
        expr += "(1)))";
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

std::string PolynomialForm::expression(){
    std::size_t sizeDen = m_denominator.size();
    std::size_t sizeNum = m_numerator.size();

    std::string expr;

    if (m_gain.isUncertain()){
        expr += "(" + m_gain.name() + "*(";
    }else {
        expr +="(" + qftbx::text::number(m_gain.nominal()) + "*(";
    }


    for (std::size_t i = 1; i < sizeNum; i++){

        if (m_numerator[i-1].isUncertain()){
            expr += "(" + m_numerator[i-1].name() + "*s^" +
                    std::to_string(sizeNum - i) + ") +";
        } else {
            expr += "(" + qftbx::text::number(m_numerator[i-1].nominal()) + "*s^" +
                    std::to_string(sizeNum - i)+ ") +";
        }
    }

    if (m_numerator.size() > 0){
        if (m_numerator.back().isUncertain()){
            expr += "(" + m_numerator.back().name() + ")) / (";
        }else{
            expr += "(" + qftbx::text::number(m_numerator.back().nominal()) + ")) / (";
        }
    } else {
        expr += "(1)) / (";
    }

    for (std::size_t i = 1; i < sizeDen; i++){

        if (m_denominator[i-1].isUncertain()){
            expr += "(" + m_denominator[i-1].name() + "*s^" +
                    std::to_string(sizeDen - i) + ") +";
        } else {
            expr += "(" + qftbx::text::number(m_denominator[i-1].nominal()) + "*s^" +
                    std::to_string(sizeDen - i) + ") +";
        }
    }

    if (m_denominator.size() > 0){
        if (m_denominator.back().isUncertain()){
            expr += "(" + m_denominator.back().name() + ")))";
        }else{
            expr += "(" + qftbx::text::number(m_denominator.back().nominal()) + ")))";
        }
    } else {
        expr += "(1)))";
    }

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}

std::complex <double> PolynomialForm::evaluateNumerator(std::vector <double> * nume, double omega){

    if (nume->size() == 0){
        return std::complex <double> (1, 0);
    }

    std::size_t sizeNum = nume->size();
    std::string expr = "(";


    for (std::size_t i = 1; i < sizeNum; i++){
        expr += "(" + qftbx::text::number(nume->at(i-1)) + "*(" + qftbx::text::number(omega) + "*i)^" +
                std::to_string(sizeNum - i)+ ") +";
    }

    expr += "(" + qftbx::text::number(nume->back()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr);

    return p.Eval().GetComplex();
}

std::complex <double> PolynomialForm::evaluateDenominator(std::vector <double> * deno, double omega){

    if (deno->size() == 0){
        return std::complex <double> (1, 0);
    }

    std::size_t sizeDen = deno->size();
    std::string expr = "(";


    for (std::size_t i = 1; i < sizeDen; i++){
        expr += "(" + qftbx::text::number(deno->at(i-1)) + "*(" + qftbx::text::number(omega) + "*i)^" +
                std::to_string(sizeDen - i)+ ") +";
    }

    expr += "(" + qftbx::text::number(deno->back()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr);

    return p.Eval().GetComplex();
}


//P(s) = k * (a[0]*s^(n-1) + ... + a[n-1]) / (b[0]*s^(m-1) + ... + b[m-1]),
//at s = j*w, times the pure delay. Evaluated by Horner, which is both the
//accurate way to sum a polynomial and needs no pow() on a complex base; an
//empty list is the constant 1, as the expression generator writes it.
std::complex <double> PolynomialForm::valueAt(double w, const std::vector<double> & numerator,
                                             const std::vector<double> & denominator,
                                             double gain, double delay)
{
    const std::complex<double> s(0.0, w);

    std::complex<double> num(1.0, 0.0);
    if (!numerator.empty()) {
        num = std::complex<double>(numerator.front(), 0.0);
        for (std::size_t i = 1; i < numerator.size(); i++) {
            num = num * s + numerator[i];
        }
    }

    std::complex<double> den(1.0, 0.0);
    if (!denominator.empty()) {
        den = std::complex<double>(denominator.front(), 0.0);
        for (std::size_t i = 1; i < denominator.size(); i++) {
            den = den * s + denominator[i];
        }
    }

    //exp(0) is exactly 1, so a zero delay needs no special case.
    return gain * num / den * std::exp(-s * delay);
}

} // namespace qftbx
