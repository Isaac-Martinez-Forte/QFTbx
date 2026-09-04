#include <regex>
#include <string>
#include <algorithm>
#include <vector>
#include <cstdint>
#include "free_form.h"

#include <cmath>
#include <complex>


#include "src/core/text_tokens.h"
#include "src/core/exception.h"
#include "src/core/math/expression_cache.h"

namespace qftbx {

FreeForm::FreeForm(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k,
                           Parameter delay, std::string numeratorExpr, std::string denominatorExpr)
    :TransferFunction (name, std::move(numerator), std::move(denominator), std::move(k), std::move(delay))
{
    m_numeratorExpr = numeratorExpr;
    m_denominatorExpr = denominatorExpr;

    //Built HERE and never again. It was a lazily filled mutable member, and
    //valueAt() runs on one plant from several OpenMP threads: two of them saw
    //it empty, both built it and both assigned a std::string - a refcounted
    //pointer swap - which ThreadSanitizer caught as a data race. Doing it
    //once in the constructor costs two regular expressions per plant and
    //leaves nothing shared to race on.
    //
    //Only the standalone Laplace variable is replaced: a plain substring
    //replace mutilated "sin", "sqrt", "abs" and any parameter whose name
    //contains an 's'.
    static const std::regex laplaceVariable("\\bs\\b");

    std::string numeratorText = m_numeratorExpr;
    std::string denominatorText = m_denominatorExpr;
    numeratorText = std::regex_replace(numeratorText, laplaceVariable, laplaceName());
    denominatorText = std::regex_replace(denominatorText, laplaceVariable, laplaceName());

    m_boundExpression = "(" + numeratorText + ")/(" + denominatorText + ")";
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
//have to be evaluated as expressions - but neither the frequency nor the
//coefficients need to travel as TEXT. The Laplace variable is replaced by a
//BOUND variable and the named coefficients are bound to their values, so
//nothing goes through qftbx::text::number() and its six significant digits. The
//gain and the delay arrive already reduced to values (Parameter::nominal()
//has applied any reparametrisation), so their own expressions are not
//re-evaluated here either.
std::complex <double> FreeForm::valueAt(double w, const std::vector<double> & numerator,
                                       const std::vector<double> & denominator,
                                       double gain, double delay)
{
    //One value per DISTINCT name. A name appearing more than once is ONE
    //variable, not several - the cervera plant carries its "a" in both the
    //numerator and the denominator - so every appearance must be given the
    //same value. A disagreement means the caller built an inconsistent
    //request; picking one of the two would evaluate a plant nobody asked
    //for, so it is reported.
    std::vector<std::string> names;
    std::vector<std::complex<double>> bound;

    names.push_back(laplaceName());
    bound.push_back(std::complex<double>(0.0, w));

    const auto remember = [&](const std::vector<Parameter> & parameters, const std::vector<double> & given) {
        //One value per parameter, no more and no fewer. This used to walk to
        //the shorter of the two and say nothing, which made a caller's
        //miscount into a plant evaluated with some coefficients missing -
        //while a name given two values, one line below, was refused. The
        //same mistake deserves the same answer.
        if (parameters.size() != given.size()) {
            throw qftbx::InvalidInput("FreeForm::valueAt: " + std::to_string(given.size())
                                      + " values were given for " + std::to_string(parameters.size())
                                      + " parameters");
        }

        for (std::size_t i = 0; i < parameters.size(); i++) {
            const std::string name = parameters[i].name();
            const auto found = std::find(names.begin(), names.end(), name);

            //The iterator answers "is it there" and "where" at once, so
            //there is no index and no -1 to stand for "nowhere".
            if (found == names.end()) {
                names.push_back(name);
                bound.push_back(std::complex<double>(given[i], 0.0));
                continue;
            }

            const std::size_t at = static_cast<std::size_t>(
                        std::distance(names.begin(), found));

            if (bound[at] != std::complex<double>(given[i], 0.0)) {
                throw qftbx::InvalidInput(
                    "the parameter \"" + name + "\" was given two different values ("
                    + qftbx::text::number(bound[at].real()) + " and "
                    + qftbx::text::number(given[i])
                    + "): the same name is the same variable");
            }
        }
    };

    remember(m_numerator, numerator);
    remember(m_denominator, denominator);

    //Parsed once per thread: see qftbx::math::evaluateCached. The text no
    //longer carries the frequency, so it is the same expression on every
    //call and the cache actually hits.
    const std::complex<double> ratio =
            qftbx::math::evaluateCached(m_boundExpression, names, bound);

    const std::complex<double> s(0.0, w);

    return gain * ratio * std::exp(-s * delay);
}

//The Laplace variable as a BOUND variable name. Substituting the frequency as
//text was what rounded it to six significant digits.
const std::string & FreeForm::laplaceName()
{
    static const std::string name = ("__jw");
    return name;
}

} // namespace qftbx
