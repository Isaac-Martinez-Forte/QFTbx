#include "transfer_function.h"

#include "mpParser.h"

namespace qftbx {

TransferFunction::TransferFunction(std::string name, std::vector <Parameter> numerator,
        std::vector <Parameter> denominator, Parameter k, Parameter delay) :
LtiSystem(name),
m_gain(std::move(k)),
m_delay(std::move(delay)),
m_numerator(std::move(numerator)),
m_denominator(std::move(denominator)) {
}

std::vector <Parameter> & TransferFunction::numerator() {
    return m_numerator;
}

std::vector <Parameter> & TransferFunction::denominator() {
    return m_denominator;
}

Parameter & TransferFunction::gain() {
    return m_gain;
}

Parameter & TransferFunction::delay() {
    return m_delay;
}

std::complex <double> TransferFunction::evaluate(std::vector <double> * numerator, std::vector <double> * denominator,
        double k, double delay, double omega) {
    mup::ParserX p(mup::pckALL_COMPLEX);

    p.SetExpr(expression(numerator, denominator, k, delay, omega));

    return p.Eval().GetComplex();
}

namespace {

//The nominal of every parameter, in order. Uncertain ones contribute their
//nominal here exactly as they did through the parser, which bound each name
//to its nominal before evaluating.
std::vector<double> nominalsOf(const std::vector<Parameter> & parameters)
{
    std::vector<double> values;
    values.reserve(parameters.size());

    for (const Parameter & parameter : parameters) {
        values.push_back(parameter.nominal());
    }

    return values;
}

} // namespace

std::complex <double> TransferFunction::evaluate(double w) {
    //Direct complex arithmetic: see valueAt() in the header for what this
    //replaced and why.
    return valueAt(w, nominalsOf(m_numerator), nominalsOf(m_denominator),
                   m_gain.nominal(), m_delay.nominal());
}

std::vector <std::complex <double> > TransferFunction::evaluate(const std::vector <double> & omega) {

    std::vector <std::complex <double> > resultado;
    resultado.reserve(omega.size());

    for (double o : omega) {
        resultado.push_back(evaluate(o));
    }

    return resultado;
}

std::string TransferFunction::numeratorString() {
    return std::string();
}

std::string TransferFunction::denominatorString() {
    return std::string();
}

std::unique_ptr<LtiSystem> TransferFunction::clone() {

    //Values copy themselves: no per-parameter cloning any more. FreeForm
    //overrides this to carry its expression strings too.
    return this->create(this->name(), m_numerator, m_denominator, m_gain, m_delay);
}

} // namespace qftbx
