#include <cstdint>
#include <cmath>
#include <complex>

#include "polynomial_form.h"

using namespace std;

namespace qftbx {

PolynomialForm::PolynomialForm(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay):
    TransferFunction(name, numerator, denominator, k , delay)
{

}

PolynomialForm::~PolynomialForm(){
}

std::unique_ptr<LtiSystem> PolynomialForm::create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                               Parameter k, Parameter delay, QString numeratorExpr __attribute__((unused)), QString denominatorExpr __attribute__((unused))){
    return std::make_unique<PolynomialForm>(name, std::move(numerator), std::move(denominator),
                                            std::move(k), std::move(delay));
}

LtiSystem::SystemType PolynomialForm::type(){
    return SystemType::PolynomialForm;
}

QString PolynomialForm::expression (QVector <double> * numerator, QVector <double> * denominator,
                              double k, double delay, double omega){

    std::int32_t sizeDen = denominator->size();
    std::int32_t sizeNum = numerator->size();

    QString expr;


    expr +=  "(" +  QString::number(k) + "*(";



    for (std::int32_t i = 1; i < sizeNum; i++){


        expr += "(" + QString::number(numerator->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeNum - i)+ ") +";

    }


    if (sizeNum > 0){
        expr += "(" + QString::number(numerator->last()) + ")) / (";
    } else {
        expr += "(1))/(";
    }


    for (std::int32_t i = 1; i < sizeDen; i++){


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

QString PolynomialForm::expression(double w){

    std::int32_t sizeDen = m_denominator.size();
    std::int32_t sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += "(" + m_gain.name() + "*(";
    }else {
        expr += "(" + QString::number(m_gain.nominal()) + "*(";
    }


    for (std::int32_t i = 1; i < sizeNum; i++){

        if (m_numerator[i-1].isUncertain()){
            expr += "(" + m_numerator[i-1].name() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            expr += "(" + QString::number(m_numerator[i-1].nominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (m_numerator.size() > 0){
        if (m_numerator.back().isUncertain()){
            expr += "(" + m_numerator.back().name() + ")) / (";
        }else{
            expr += "(" + QString::number(m_numerator.back().nominal()) + ")) / (";
        }
    } else {
        expr += "(1)) / (";
    }

    for (std::int32_t i = 1; i < sizeDen; i++){

        if (m_denominator[i-1].isUncertain()){
            expr += "(" + m_denominator[i-1].name() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            expr += "(" + QString::number(m_denominator[i-1].nominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }


    if (m_denominator.size() > 0){
        if (m_denominator.back().isUncertain()){
            expr += "(" + m_denominator.back().name() + ")))";
        }else{
            expr += "(" + QString::number(m_denominator.back().nominal()) + ")))";
        }
    } else {
        expr += "(1)))";
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

QString PolynomialForm::expression(){
    std::int32_t sizeDen = m_denominator.size();
    std::int32_t sizeNum = m_numerator.size();

    QString expr;

    if (m_gain.isUncertain()){
        expr += "(" + m_gain.name() + "*(";
    }else {
        expr +="(" + QString::number(m_gain.nominal()) + "*(";
    }


    for (std::int32_t i = 1; i < sizeNum; i++){

        if (m_numerator[i-1].isUncertain()){
            expr += "(" + m_numerator[i-1].name() + "*s^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            expr += "(" + QString::number(m_numerator[i-1].nominal()) + "*s^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (m_numerator.size() > 0){
        if (m_numerator.back().isUncertain()){
            expr += "(" + m_numerator.back().name() + ")) / (";
        }else{
            expr += "(" + QString::number(m_numerator.back().nominal()) + ")) / (";
        }
    } else {
        expr += "(1)) / (";
    }

    for (std::int32_t i = 1; i < sizeDen; i++){

        if (m_denominator[i-1].isUncertain()){
            expr += "(" + m_denominator[i-1].name() + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            expr += "(" + QString::number(m_denominator[i-1].nominal()) + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }

    if (m_denominator.size() > 0){
        if (m_denominator.back().isUncertain()){
            expr += "(" + m_denominator.back().name() + ")))";
        }else{
            expr += "(" + QString::number(m_denominator.back().nominal()) + ")))";
        }
    } else {
        expr += "(1)))";
    }

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + QString::number(m_delay.nominal()) +")";
    }

    return expr;
}

std::complex <double> PolynomialForm::evaluateNumerator(QVector <double> * nume, double omega){

    if (nume->size() == 0){
        return std::complex <double> (1, 0);
    }

    std::int32_t sizeNum = nume->size();
    QString expr = "(";


    for (std::int32_t i = 1; i < sizeNum; i++){
        expr += "(" + QString::number(nume->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeNum - i)+ ") +";
    }

    expr += "(" + QString::number(nume->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

std::complex <double> PolynomialForm::evaluateDenominator(QVector <double> * deno, double omega){

    if (deno->size() == 0){
        return std::complex <double> (1, 0);
    }

    std::int32_t sizeDen = deno->size();
    QString expr = "(";


    for (std::int32_t i = 1; i < sizeDen; i++){
        expr += "(" + QString::number(deno->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeDen - i)+ ") +";
    }

    expr += "(" + QString::number(deno->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(expr.toStdString());

    return p.Eval().GetComplex();
}

} // namespace qftbx

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
