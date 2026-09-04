#ifndef QFTBX_PARAMETER_H
#define QFTBX_PARAMETER_H

#include <string>

#include "src/core/range.h"

namespace qftbx {

/**
 * @brief A plant parameter: either a plain constant or an uncertain
 * parameter with a name, a nominal value and a range [min, max].
 *
 * An optional expression reparametrises the value: nominal() and range()
 * evaluate it with muParserX substituting the raw value (rawNominal() and
 * rawRange() return the untransformed ones). Ranges given inverted are
 * normalised on construction.
 */
class Parameter
{
public:
    /// Uncertain parameter; an empty exp falls back to the name.
    Parameter(std::string name, Range range, double nominal, std::string exp);

    /// Uncertain parameter without reparametrisation.
    Parameter(std::string name, Range range, double nominal);

    Parameter();

    /// Constant, named by its textual value.
    Parameter (double value);

    /// Named constant.
    Parameter (std::string name, double value);

    void setName(std::string name);

    /// True for uncertain parameters, false for constants.
    bool isUncertain() const;

    const std::string & name() const;

    /// Range with the reparametrisation applied.
    Range range() const;

    /// Raw range, without the reparametrisation.
    Range rawRange() const;

    /// Nominal value with the reparametrisation applied.
    double nominal() const;

    /// Raw nominal value, without the reparametrisation.
    double rawNominal() const;

    const std::string & expression() const;

    /**
     * @brief Value equality, on the RAW state: name, raw range, raw nominal,
     * reparametrisation expression and the two flags.
     *
     * Deliberately conservative and deliberately total - every member is
     * compared. It exists so the project can tell a real change from a
     * dialog accepted without an edit, and the two answers are not
     * symmetric: saying "equal" when something did change leaves the
     * templates computed for the OLD plant in place, which is the silent
     * defect ProjectController warns about. Saying "different" when nothing
     * changed only costs a recomputation. So anything not compared here
     * would have to be proven irrelevant first, and nothing is.
     *
     * The raw values are the ones compared, not the reparametrised ones:
     * they are the state, and the transformed ones are derived from them.
     */
    bool operator==(const Parameter & other) const;

    bool operator!=(const Parameter & other) const { return !(*this == other); }

private:
    /// The reparametrisation applied to one value, parsed once per thread.
    double realValueOf(double value) const;

    //Initialised here, not constructor by constructor: the value
    //constructors used to leave m_hasExpression indeterminate, and reading
    //it (the copy constructor does) is undefined behaviour. It stayed
    //harmless only because range() and nominal() return early on
    //!m_uncertain, so a single setUncertain(true) - or swapping those two
    //checks - would have turned a stack byte into a muParserX evaluation of
    //an undefined variable.
    std::string m_name;
    Range m_range;
    double m_nominal = 0.0;
    bool m_uncertain = false;
    std::string m_expression;
    bool m_hasExpression = false;

};

} // namespace qftbx


#endif // QFTBX_PARAMETER_H
