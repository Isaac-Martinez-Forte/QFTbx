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
#include "src/core/math/polynomial.h"

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
    //may call it from every thread at once. Nothing parses per evaluation,
    //and nothing renames the Laplace variable to keep it clear of a
    //library's own names.
    std::unique_ptr<ExpressionTree> ratio;
    std::unique_ptr<ExpressionTree> denominatorTree;
    try {
        ratio = std::make_unique<ExpressionTree>(
                    "(" + m_numeratorExpr + ")/(" + m_denominatorExpr + ")");
        denominatorTree = std::make_unique<ExpressionTree>("(" + m_denominatorExpr + ")");
    } catch (const std::invalid_argument & error) {
        throw InvalidInput(QFTBX_TR("Core", "The plant expression cannot be read: %1").arg(error.what()));
    }

    bindNames(*ratio, *denominatorTree);
    m_ratio = std::move(ratio);
    m_denominatorTree = std::move(denominatorTree);
}

//The order of the values valueAt() is given: the Laplace variable first,
//then every distinct parameter name in the order of the numerator and the
//denominator. A name appearing more than once is ONE variable, not several
//(the cervera plant carries its "a" in both), so it takes one slot and
//every appearance points at it.
void FreeForm::bindNames(ExpressionTree & ratio, ExpressionTree & denominator)
{
    std::vector<std::string> names;
    names.push_back(laplaceName());

    const auto slotOf = [&](const Parameter & parameter) {
        if (parameter.name() == laplaceName()) {
            throw InvalidInput(QFTBX_TR("Core", "A plant parameter cannot be called \"%1\": that is the Laplace variable.").arg(laplaceName()));
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
        denominator.bind(names);
    } catch (const std::invalid_argument & error) {
        throw InvalidInput(QFTBX_TR("Core", "The plant expression cannot be evaluated: %1").arg(error.what()));
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

    std::unique_ptr<LtiSystem> copy = this->create(this->name(), m_numerator, m_denominator,
                                                   m_gain, m_delay,
                                                   m_numeratorExpr, m_denominatorExpr);
    copy->setDescription(this->description());

    return copy;
}

std::vector<std::complex<double>> FreeForm::boundValues(const std::vector<double> & numerator,
                                                        const std::vector<double> & denominator) const
{
    //One value per parameter, no more and no fewer: walking to the shorter
    //of the two turns a caller's miscount into a plant evaluated with some
    //coefficients missing, and says nothing.
    if (numerator.size() != m_numeratorSlots.size() || denominator.size() != m_denominatorSlots.size()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "FreeForm::valueAt: %1 and %2 values were given for %3 and %4 parameters")
                                  .arg(numerator.size()).arg(denominator.size()).arg(m_numeratorSlots.size()).arg(m_denominatorSlots.size()));
    }

    std::vector<std::complex<double>> values(m_valueCount);
    std::vector<char> filled(m_valueCount, 0);

    filled[0] = 1;   //slot 0 is the caller's

    //A name given two different values means the caller built an
    //inconsistent request; picking one of the two would evaluate a plant
    //nobody asked for, so it is reported.
    const auto place = [&](const std::vector<Parameter> & parameters, const std::vector<std::size_t> & slots,
                           const std::vector<double> & given) {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            const std::size_t slot = slots[i];
            const std::complex<double> value(given[i], 0.0);

            if (filled[slot] && values[slot] != value) {
                throw qftbx::InvalidInput(QFTBX_TR("Core", "the parameter \"%1\" was given two different values (%2 and %3): the same name is the same variable")
                    .arg(parameters[i].name()).arg(values[slot].real()).arg(given[i]));
            }

            values[slot] = value;
            filled[slot] = 1;
        }
    };

    place(m_numerator, m_numeratorSlots, numerator);
    place(m_denominator, m_denominatorSlots, denominator);

    return values;
}

//The denominator expression evaluated on its own gives the polynomial whose
//roots are the poles - when it is one. A delay or a transcendental written
//into the denominator is not, and then there is no answer to give.
std::optional<std::vector<std::complex<double>>> FreeForm::polesAt(const std::vector<double> & numerator,
                                                                   const std::vector<double> & denominator)
{
    std::vector<std::complex<double>> values = boundValues(numerator, denominator);

    const auto denominatorAt = [&](std::complex<double> s) {
        values[0] = s;
        return m_denominatorTree->evaluate(values);
    };

    const std::optional<std::vector<double>> coefficients = math::polynomialCoefficients(denominatorAt);
    if (!coefficients.has_value()) {
        return std::nullopt;
    }

    return math::polynomialRoots(*coefficients);
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
    std::vector<std::complex<double>> values = boundValues(numerator, denominator);
    values[0] = std::complex<double>(0.0, w);

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
