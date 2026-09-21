/**
 * @file
 * @brief Nominal evaluation and cloning shared by the transfer-function forms.
 *
 * The nominal value at a frequency is the shape evaluated at the nominal of
 * every parameter, in order, with any reparametrisation already applied by
 * the parameters themselves. A clone is made through the virtual
 * constructor with the parameter values, which copy themselves, and then
 * carries the description across, since a copy that lost it would be a
 * different plant on screen. The textual numerator and denominator are
 * empty here; only the free form fills them.
 */

#include "src/core/system/transfer_function.h"

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

namespace {

std::vector<double> nominalsOf(const std::vector<Parameter> & parameters)
{
    std::vector<double> values;
    values.reserve(parameters.size());

    for (const Parameter & parameter : parameters) {
        values.push_back(parameter.nominal());
    }

    return values;
}

}

std::complex <double> TransferFunction::evaluate(double w) {
    return valueAt(w, nominalsOf(m_numerator), nominalsOf(m_denominator),
                   m_gain.nominal(), m_delay.nominal());
}

std::optional<std::vector<std::complex<double>>> TransferFunction::nominalPoles() {
    return polesAt(nominalsOf(m_numerator), nominalsOf(m_denominator));
}

std::vector <std::complex <double> > TransferFunction::evaluate(const std::vector <double> & omega) {

    std::vector <std::complex <double> > values;
    values.reserve(omega.size());

    for (double o : omega) {
        values.push_back(evaluate(o));
    }

    return values;
}

std::string TransferFunction::numeratorString() {
    return std::string();
}

std::string TransferFunction::denominatorString() {
    return std::string();
}

std::unique_ptr<LtiSystem> TransferFunction::clone() {

    std::unique_ptr<LtiSystem> copy = this->create(this->name(), m_numerator, m_denominator,
                                                   m_gain, m_delay);
    copy->setDescription(this->description());

    return copy;
}

}
