/**
 * @file
 * @brief Evaluation of a free-form plant through its parsed expression.
 *
 * Nothing is parsed per evaluation: the value vector is filled from the
 * given coefficients by slot, with the Laplace variable in slot 0, and the
 * tree is read from as many threads as the template sweep runs. A name
 * appearing in both polynomials is one variable, so two different values
 * for it are refused rather than one being picked, and a value count that
 * does not match the parameters is refused rather than truncated. The gain
 * and the delay arrive as values and are applied outside the tree. The
 * poles come from recovering the denominator's polynomial by evaluation;
 * a denominator that is not a polynomial in s gives no answer.
 */

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
    if (numerator.size() != m_numeratorSlots.size() || denominator.size() != m_denominatorSlots.size()) {
        throw qftbx::InvalidInput(QFTBX_TR("Core", "FreeForm::valueAt: %1 and %2 values were given for %3 and %4 parameters")
                                  .arg(numerator.size()).arg(denominator.size()).arg(m_numeratorSlots.size()).arg(m_denominatorSlots.size()));
    }

    std::vector<std::complex<double>> values(m_valueCount);
    std::vector<char> filled(m_valueCount, 0);

    filled[0] = 1;

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

const std::string & FreeForm::laplaceName()
{
    static const std::string name("s");
    return name;
}

std::optional<LtiSystem::Polynomials> FreeForm::polynomialsAt(const std::vector<double> & numerator,
                                                              const std::vector<double> & denominator,
                                                              double gain)
{
    std::vector<std::complex<double>> values = boundValues(numerator, denominator);

    const auto denominatorAt = [&](std::complex<double> s) {
        values[0] = s;
        return m_denominatorTree->evaluate(values);
    };
    const auto numeratorAt = [&](std::complex<double> s) {
        values[0] = s;
        return m_ratio->evaluate(values) * m_denominatorTree->evaluate(values);
    };

    const std::optional<std::vector<double>> den = math::polynomialCoefficients(denominatorAt);
    const std::optional<std::vector<double>> num = math::polynomialCoefficients(numeratorAt);
    if (!den.has_value() || !num.has_value()) {
        return std::nullopt;
    }

    Polynomials polynomials{*num, *den};
    for (double & coefficient : polynomials.numerator) {
        coefficient *= gain;
    }
    return polynomials;
}

}
