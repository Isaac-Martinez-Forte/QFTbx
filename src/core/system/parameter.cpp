#include "src/core/system/parameter.h"

#include <cmath>

#include "src/core/common/text_tokens.h"

#include "src/core/common/exception.h"
#include "src/core/math/expression_cache.h"

namespace qftbx {

namespace {

//muParserX does not complain about a degenerate expression: "0/0", "1/0",
//"log(-1)" and "sqrt(-1)" all evaluate quietly to a NaN or an infinity
//(measured, not assumed). Every one of those values used to sail straight
//into the model from a dialog field, and nothing downstream looks: the
//templates come out non-finite, so do the boundaries, the plot is empty and
//the search never converges - without one message anywhere saying why.
//A parameter is the choke point every uncertainty bound and nominal value
//goes through, so the check belongs here.
void requireFinite(double value, const char * what)
{
    if (!std::isfinite(value)) {
        throw InvalidInput(std::string("A parameter's ") + what +
                           " must be a finite number.");
    }
}

void requireFiniteRange(const Range & range)
{
    requireFinite(range.min, "range start");
    requireFinite(range.max, "range end");
}

}

//setRange(), setNominal(), setUncertain() and Parameter(Range) are gone. The
//last three had no caller at all, and setRange() had exactly one - a branch of
//the .qft reader for a "historical quirk" of a dialect that no longer exists -
//and it stored a range without the ordering and the finiteness checks every
//constructor performs, so it was the one door a NaN or an inverted range could
//still come in through. A Parameter is built valid and stays valid.
bool Parameter::operator==(const Parameter & other) const
{
    return m_name == other.m_name &&
            rawRange() == other.rawRange() &&
            rawNominal() == other.rawNominal() &&
            m_expression == other.m_expression &&
            m_uncertain == other.m_uncertain &&
            m_hasExpression == other.m_hasExpression;
}

Parameter::Parameter(std::string name, Range range, double nominal, std::string exp)
{
    requireFiniteRange(range);
    requireFinite(nominal, "nominal value");

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
    requireFiniteRange(range);
    requireFinite(nominal, "nominal value");

    m_name = name;

    m_range = range.ordered();

    m_nominal = nominal;
    m_expression = name;

    m_uncertain = true;


    m_hasExpression = false;
}

Parameter::Parameter() {
   m_nominal = 0;
   m_uncertain = false;
   m_hasExpression = false;
}

Parameter::Parameter (double value){
    requireFinite(value, "value");

    m_nominal = value;
    m_name = qftbx::text::number(m_nominal);
    m_uncertain = false;
    m_range = Range(m_nominal, m_nominal);
    m_expression = m_name;
}

Parameter::Parameter (std::string name, double value){
    requireFinite(value, "value");

    m_nominal = value;
    m_name = name;
    m_uncertain = false;
    m_range = Range(m_nominal, m_nominal);
    m_expression = name;
}


bool Parameter::isUncertain() const {
    return m_uncertain;
}

const std::string & Parameter::name() const {
    return m_name;
}

Range Parameter::range() const {

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

double Parameter::nominal() const {

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

const std::string & Parameter::expression() const {
    return m_expression;
}

Range Parameter::rawRange() const {
    return m_range;
}

double Parameter::rawNominal() const {
    return m_nominal;
}


} // namespace qftbx
