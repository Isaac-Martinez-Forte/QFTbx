#include <string>
#include <algorithm>
#include <vector>
#include <cstdint>
#include "src/core/system/free_form.h"

#include <cmath>
#include <complex>
#include <stdexcept>

#include "src/core/common/text_tokens.h"
#include "src/core/common/exception.h"

namespace qftbx {

FreeForm::FreeForm(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k,
                           Parameter delay, std::string numeratorExpr, std::string denominatorExpr)
    :TransferFunction (name, std::move(numerator), std::move(denominator), std::move(k), std::move(delay))
{
    m_numeratorExpr = numeratorExpr;
    m_denominatorExpr = denominatorExpr;

    //Parsed HERE, once, and bound to the Laplace variable and the distinct
    //parameter names: valueAt() then evaluates the tree from a vector of
    //values, which reads the tree and writes nothing, so the template sweep
    //may call it from every thread at once. The text used to be handed to an
    //expression library on every evaluation, with the Laplace variable
    //renamed by a regular expression to keep it out of the way of the
    //library's own names.
    std::unique_ptr<ExpressionTree> ratio;
    try {
        ratio = std::make_unique<ExpressionTree>(
                    "(" + m_numeratorExpr + ")/(" + m_denominatorExpr + ")");
    } catch (const std::invalid_argument & error) {
        throw InvalidInput(std::string("The plant expression cannot be read: ") + error.what());
    }

    bindNames(*ratio);
    m_ratio = std::move(ratio);
}

//The order of the values valueAt() is given: the Laplace variable first,
//then every distinct parameter name in the order of the numerator and the
//denominator. A name appearing more than once is ONE variable, not several
//(the cervera plant carries its "a" in both), so it takes one slot and
//every appearance points at it.
void FreeForm::bindNames(ExpressionTree & ratio)
{
    std::vector<std::string> names;
    names.push_back(laplaceName());

    const auto slotOf = [&](const Parameter & parameter) {
        if (parameter.name() == laplaceName()) {
            throw InvalidInput("A plant parameter cannot be called \"" + laplaceName()
                               + "\": that is the Laplace variable.");
        }

        const auto found = std::find(names.begin(), names.end(), parameter.name());
        if (found != names.end()) {
            return static_cast<std::size_t>(std::distance(names.begin(), found));
        }

        names.push_back(parameter.name());
        return names.size() - 1;
    };

    m_numeratorSlots.clear();
    for (const Parameter & parameter : m_numerator) {
        m_numeratorSlots.push_back(slotOf(parameter));
    }

    m_denominatorSlots.clear();
    for (const Parameter & parameter : m_denominator) {
        m_denominatorSlots.push_back(slotOf(parameter));
    }

    //Every variable of the expression must be one of the parameters: a
    //name the plant does not declare would evaluate to nothing.
    try {
        ratio.bind(names);
    } catch (const std::invalid_argument & error) {
        throw InvalidInput(std::string("The plant expression cannot be evaluated: ") + error.what());
    }

    m_valueCount = names.size();
}

std::string FreeForm::expression(){
    std::string expr = m_gain.expression() + "*(" + m_numeratorExpr + ")/(" + m_denominatorExpr + ")";

    if (m_delay.isUncertain()){
        expr += " * e^(-s*" + m_delay.name() + ")";
    }else if (m_delay.nominal() != 0){
        expr += " * e^(-s*" + qftbx::text::number(m_delay.nominal()) +")";
    }

    return expr;
}

LtiSystem::SystemType FreeForm::type(){
    return SystemType::FreeForm;
}

std::unique_ptr<LtiSystem> FreeForm::create(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                               Parameter k, Parameter delay, std::string numeratorExpr, std::string denominatorExpr){

    return std::make_unique<FreeForm>(name, std::move(numerator), std::move(denominator),
                                     std::move(k), std::move(delay), numeratorExpr,
                                     denominatorExpr);
}


std::string FreeForm::numeratorString(){
    return m_numeratorExpr;
}

std::string FreeForm::denominatorString(){
    return m_denominatorExpr;
}

std::unique_ptr<LtiSystem> FreeForm::clone(){

    return this->create(this->name(), m_numerator, m_denominator, m_gain, m_delay,
                        m_numeratorExpr, m_denominatorExpr);
}

//A free-form plant is written by the user, so its numerator and denominator
//are evaluated as an expression - but neither the frequency nor the
//coefficients travel as text: the Laplace variable and the named
//coefficients are bound to their values. The gain and the delay arrive
//already reduced to values (Parameter::nominal() has applied any
//reparametrisation), so their own expressions are not re-evaluated here
//either.
std::complex <double> FreeForm::valueAt(double w, const std::vector<double> & numerator,
                                       const std::vector<double> & denominator,
                                       double gain, double delay)
{
    //One value per parameter, no more and no fewer. This used to walk to
    //the shorter of the two and say nothing, which made a caller's miscount
    //into a plant evaluated with some coefficients missing.
    if (numerator.size() != m_numeratorSlots.size() || denominator.size() != m_denominatorSlots.size()) {
        throw qftbx::InvalidInput("FreeForm::valueAt: " + std::to_string(numerator.size()) + " and "
                                  + std::to_string(denominator.size()) + " values were given for "
                                  + std::to_string(m_numeratorSlots.size()) + " and "
                                  + std::to_string(m_denominatorSlots.size()) + " parameters");
    }

    std::vector<std::complex<double>> values(m_valueCount);
    std::vector<char> filled(m_valueCount, 0);

    values[0] = std::complex<double>(0.0, w);
    filled[0] = 1;

    //A name given two different values means the caller built an
    //inconsistent request; picking one of the two would evaluate a plant
    //nobody asked for, so it is reported.
    const auto place = [&](const std::vector<Parameter> & parameters, const std::vector<std::size_t> & slots,
                           const std::vector<double> & given) {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            const std::size_t slot = slots[i];
            const std::complex<double> value(given[i], 0.0);

            if (filled[slot] && values[slot] != value) {
                throw qftbx::InvalidInput(
                    "the parameter \"" + parameters[i].name() + "\" was given two different values ("
                    + qftbx::text::number(values[slot].real()) + " and "
                    + qftbx::text::number(given[i])
                    + "): the same name is the same variable");
            }

            values[slot] = value;
            filled[slot] = 1;
        }
    };

    place(m_numerator, m_numeratorSlots, numerator);
    place(m_denominator, m_denominatorSlots, denominator);

    const std::complex<double> ratio = m_ratio->evaluate(values);

    const std::complex<double> s(0.0, w);

    return gain * ratio * std::exp(-s * delay);
}

//The Laplace variable, as the user writes it.
const std::string & FreeForm::laplaceName()
{
    static const std::string name("s");
    return name;
}

} // namespace qftbx
