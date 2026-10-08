/**
 * @file
 * @brief A plant parameter: a constant or an uncertain value with a range.
 *
 * Declares the parameter with its name, nominal value and range [min, max],
 * and an optional reparametrisation expression that maps the raw values to
 * the ones the design sees: nominal() and range() evaluate it with the raw
 * value bound to the name, rawNominal() and rawRange() do not. An empty
 * expression, or one that is just the name, is the identity. Construction
 * throws qftbx::InvalidInput on a value that is not finite or an
 * expression that cannot be read; inverted ranges are normalised, and a
 * constant built from a value is named by its text. Value equality
 * compares the raw state in full, every member, conservatively: a false
 * "equal" would keep templates computed for another plant while a false
 * "different" only costs a recomputation, so a member left out would first
 * have to be proven irrelevant. The parsed reparametrisation is shared
 * between the copies the searches make by the million, and is null for the
 * identity. Every member is initialised where it is declared, so no
 * constructor can leave one indeterminate for a copy to read.
 */

#ifndef QFTBX_PARAMETER_H
#define QFTBX_PARAMETER_H

#include <memory>
#include <string>

#include "src/core/math/range.h"

namespace qftbx {

class Parameter
{
public:
    Parameter(std::string name, Range range, double nominal, std::string exp);

    Parameter(std::string name, Range range, double nominal);

    Parameter();

    Parameter (double value);

    Parameter (std::string name, double value);

    void setName(std::string name);

    bool isUncertain() const;

    const std::string & name() const;

    Range range() const;

    Range rawRange() const;

    double nominal() const;

    double rawNominal() const;

    const std::string & expression() const;

    bool operator==(const Parameter & other) const;

    bool operator!=(const Parameter & other) const { return !(*this == other); }

private:
    double realValueOf(double value) const;

    void compileExpression();

    bool identityExpression() const;

    std::string m_name;
    Range m_range;
    double m_nominal = 0.0;
    bool m_uncertain = false;
    std::string m_expression;
    bool m_hasExpression = false;

    std::shared_ptr<const class ExpressionTree> m_compiled;

};

}

#endif
