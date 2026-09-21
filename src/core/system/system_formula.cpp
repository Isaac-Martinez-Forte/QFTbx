/**
 * @file
 * @brief Builds the formula of a system family by family.
 *
 * A root at the origin is written s and not (s + 0); a zero coefficient
 * drops its term and a unit coefficient drops its factor, while an
 * uncertain one keeps both since its value is not known to be either. An
 * empty polynomial is the constant 1, as the evaluation treats it. A unit
 * gain and a zero delay are not written. The free form's texts are read
 * back through the grammar that accepted them, and text the grammar cannot
 * read is shown as it stands rather than swallowed. The interval reading
 * shows the range with the reparametrisation applied, which is what the
 * sweep walks.
 */

#include "src/core/system/system_formula.h"

#include <cmath>
#include <string>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/system/parameter.h"

namespace qftbx {

namespace {

const char kLaplace[] = "s";

bool isValue(Parameter & parameter, double value)
{
    return !parameter.isUncertain() && parameter.rawNominal() == value;
}

Formula negated(Formula inside)
{
    return formula::row({formula::number("-"), std::move(inside)});
}

Formula powerOfS(int power)
{
    if (power <= 0) {
        return Formula();
    }
    if (power == 1) {
        return formula::symbol(kLaplace);
    }

    return formula::power(formula::symbol(kLaplace), formula::number(std::to_string(power)));
}

Formula rootFactor(Parameter & parameter, int digits, ShowUncertain how)
{
    if (isValue(parameter, 0.0)) {
        return formula::symbol(kLaplace);
    }

    return formula::fenced(formula::sum(formula::symbol(kLaplace),
                                        formulaOf(parameter, digits, how)));
}

Formula constantFactor(Parameter & parameter, int digits, ShowUncertain how)
{
    return formula::fenced(formula::sum(formula::number("1"),
                                        formula::fraction(formula::symbol(kLaplace),
                                                          formulaOf(parameter, digits, how))));
}

Formula polynomial(std::vector<Parameter> & coefficients, int digits, ShowUncertain how)
{
    std::vector<Formula> terms;

    const int degree = static_cast<int>(coefficients.size()) - 1;

    for (int i = 0; i <= degree; ++i) {
        Parameter & coefficient = coefficients[std::size_t(i)];
        const int power = degree - i;

        if (isValue(coefficient, 0.0)) {
            continue;
        }

        Formula term = powerOfS(power);

        if (formula::isEmpty(term)) {
            term = formulaOf(coefficient, digits, how);
        } else if (!isValue(coefficient, 1.0)) {
            term = formula::product(formulaOf(coefficient, digits, how), std::move(term));
        }

        terms.push_back(std::move(term));
    }

    if (terms.empty()) {
        return formula::number("1");
    }

    return formula::sumOf(std::move(terms));
}

Formula factors(std::vector<Parameter> & coefficients, int digits, ShowUncertain how,
                Formula (*factor)(Parameter &, int, ShowUncertain))
{
    std::vector<Formula> pieces;
    pieces.reserve(coefficients.size());

    for (Parameter & coefficient : coefficients) {
        pieces.push_back(factor(coefficient, digits, how));
    }

    return formula::productOf(std::move(pieces));
}

Formula quotient(LtiSystem & system, int digits, ShowUncertain how)
{
    switch (system.type()) {
    case LtiSystem::SystemType::ZeroPoleGain:
        return formula::fraction(factors(system.numerator(), digits, how, rootFactor),
                                 factors(system.denominator(), digits, how, rootFactor));

    case LtiSystem::SystemType::TimeConstantGain:
        return formula::fraction(factors(system.numerator(), digits, how, constantFactor),
                                 factors(system.denominator(), digits, how, constantFactor));

    case LtiSystem::SystemType::PolynomialForm:
        return formula::fraction(polynomial(system.numerator(), digits, how),
                                 polynomial(system.denominator(), digits, how));

    case LtiSystem::SystemType::FreeForm:
        break;
    }

    const std::string numerator = system.numeratorString();
    const std::string denominator = system.denominatorString();

    Formula above = formulaOfText(numerator, digits);
    Formula below = formulaOfText(denominator, digits);

    if (formula::isEmpty(above)) {
        above = formula::number(numerator.empty() ? "1" : numerator);
    }
    if (formula::isEmpty(below)) {
        below = formula::number(denominator.empty() ? "1" : denominator);
    }

    return formula::fraction(std::move(above), std::move(below));
}

}

Formula formulaOf(Parameter & parameter, int digits, ShowUncertain how)
{
    if (parameter.isUncertain()) {
        if (how == ShowUncertain::ByRange) {
            const Range range = parameter.range();
            return formula::interval(range.min, range.max, digits);
        }
        return formula::symbol(parameter.name());
    }

    return formula::number(parameter.rawNominal(), digits);
}

bool hasUncertainty(LtiSystem & system)
{
    for (std::vector<Parameter> * polynomial : {&system.numerator(), &system.denominator()}) {
        for (Parameter & parameter : *polynomial) {
            if (parameter.isUncertain()) {
                return true;
            }
        }
    }

    return system.gain().isUncertain() || system.delay().isUncertain();
}

Formula formulaOf(LtiSystem & system, int digits, ShowUncertain how)
{
    Formula transfer = quotient(system, digits, how);

    if (!isValue(system.gain(), 1.0)) {
        transfer = formula::product(formulaOf(system.gain(), digits, how), std::move(transfer));
    }

    Parameter & delay = system.delay();
    if (delay.isUncertain() || delay.rawNominal() != 0.0) {
        Formula exponent = negated(formula::product(formulaOf(delay, digits, how),
                                                    formula::symbol(kLaplace)));
        transfer = formula::product(std::move(transfer),
                                    formula::power(formula::number("e"), std::move(exponent)));
    }

    return transfer;
}

}
