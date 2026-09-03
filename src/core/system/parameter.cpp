#include "parameter.h"

#include "src/core/text_tokens.h"

#include "src/core/exception.h"
#include "src/core/math/expression_cache.h"

using namespace mup;
using namespace std;

namespace qftbx {

Parameter::Parameter(std::string name, Range range, double nominal, std::string exp)
{
    m_name = name;

    m_range = range.ordered();

    m_nominal = nominal;
    m_uncertain = true;

    if (exp.empty()) {
        //Without a reparametrisation the expression is the parameter
        //itself, as in the three-argument constructor.
        m_expression = name;
        m_hasExpression = false;
    } else {
        m_expression = exp;
        m_hasExpression = true;
    }
}

Parameter::Parameter(std::string name, Range range, double nominal){
    m_name = name;

    m_range = range.ordered();

    m_nominal = nominal;
    m_expression = name;

    m_uncertain = true;


    m_hasExpression = false;
}

Parameter::Parameter (Range range){
    m_range = range.ordered();

    m_nominal = 0;
    m_uncertain = false;
    m_hasExpression = false;
}

Parameter::Parameter() {
   m_nominal = 0;
   m_uncertain = false;
   m_hasExpression = false;
}

Parameter::Parameter (double value){
    m_nominal = value;
    m_name = qftbx::text::number(m_nominal);
    m_uncertain = false;
    m_range = Range(m_nominal, m_nominal);
    m_expression = m_name;
}

Parameter::Parameter (std::string name, double value){
    m_nominal = value;
    m_name = name;
    m_uncertain = false;
    m_range = Range(m_nominal, m_nominal);
    m_expression = name;
}


bool Parameter::isUncertain(){
    return m_uncertain;
}

void Parameter::setUncertain(bool a) {
    m_uncertain = a;
}

const std::string & Parameter::name() const {
    return m_name;
}

Range Parameter::range(){

    if (!m_uncertain){
        return m_range;
    }

    if (!m_hasExpression){
        return m_range;
    }

    //The two ends go through the SAME parsed expression: the reparametrisation
    //is parsed once per thread and only the bound value changes. This used to
    //build a parser, SetExpr it, evaluate, then RemoveVar/DefineVar and
    //evaluate again - and changing the variable set throws the RPN away, so
    //it parsed twice per call.
    Range point;

    point.min = realValueOf(m_range.min);
    point.max = realValueOf(m_range.max);

    return point;
}

//The reparametrisation applied to one value. It describes a real quantity, so
//a complex result is a malformed expression rather than something to take the
//real part of (the historical GetFloat() threw an untyped muParserX error).
double Parameter::realValueOf(double value) const
{
    const std::complex<double> evaluated = qftbx::math::evaluateCached(
            m_expression, {m_name}, {std::complex<double>(value, 0.0)});

    if (evaluated.imag() != 0.0) {
        throw InvalidInput("the reparametrisation of \"" + m_name
                           + "\" produced a complex value");
    }

    return evaluated.real();
}

double Parameter::nominal(){

    if (!m_uncertain){
        return m_nominal;
    }

    if (!m_hasExpression){
        return m_nominal;
    }

    return realValueOf(m_nominal);
}

void Parameter::setName(std::string name){
    m_name = name;
}

void Parameter::setRange(Range range){
    m_range = range;
}

void Parameter::setNominal(double nominal){
    m_nominal = nominal;
}

const std::string & Parameter::expression() const {
    return m_expression;
}

Range Parameter::rawRange(){
    return m_range;
}

double Parameter::rawNominal(){
    return m_nominal;
}



} // namespace qftbx
