/**
 * @file
 * @brief Construction, validation and reparametrisation of a parameter.
 *
 * Every constructor refuses a non-finite value or range end, and there are
 * no setters, because a parameter is the choke point every uncertainty
 * bound and nominal value goes through and a NaN let in here reaches the
 * templates, the boundaries and the search without a message anywhere. The
 * identity reparametrisation, a parameter mapped by its own name, needs no
 * parser and is answered from the raw values; anything else is parsed once
 * and evaluated with the raw value bound to the parameter's name, both ends
 * of the range going through the same parsed expression.
 */

#include "src/core/system/parameter.h"

#include <cmath>

#include "src/core/common/text_tokens.h"

#include "src/core/common/exception.h"
#include "src/core/math/expression_tree.h"

#include <stdexcept>
#include <vector>

namespace qftbx {

namespace {

void requireFinite(double value, const char * what)
{
    if (!std::isfinite(value)) {
        throw InvalidInput(QFTBX_TR("Core", "A parameter's %1 must be a finite number.").arg(what));
    }
}

void requireFiniteRange(const Range & range)
{
    requireFinite(range.min, "range start");
    requireFinite(range.max, "range end");
}

}

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
        m_expression = name;
        m_hasExpression = false;
    } else {
        m_expression = exp;
        m_hasExpression = true;
    }

    compileExpression();
}

void Parameter::compileExpression()
{
    if (!m_hasExpression || identityExpression()) {
        m_compiled.reset();
        return;
    }

    try {
        auto compiled = std::make_shared<ExpressionTree>(m_expression);
        compiled->bind({m_name});
        m_compiled = std::move(compiled);
    } catch (const std::invalid_argument & error) {
        throw InvalidInput(QFTBX_TR("Core", "the reparametrisation of \"%1\" cannot be read: %2").arg(m_name).arg(error.what()));
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

bool Parameter::identityExpression() const
{
    return m_expression == m_name;
}

Range Parameter::range() const {

    if (!m_uncertain){
        return m_range;
    }

    if (!m_hasExpression || identityExpression()){
        return m_range;
    }

    Range point;

    point.min = realValueOf(m_range.min);
    point.max = realValueOf(m_range.max);

    return point;
}

double Parameter::realValueOf(double value) const
{
    return m_compiled->evaluate(std::vector<double>{value});
}

double Parameter::nominal() const {

    if (!m_uncertain){
        return m_nominal;
    }

    if (!m_hasExpression || identityExpression()){
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

}
