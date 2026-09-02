#ifndef QFTBX_PARAMETER_H
#define QFTBX_PARAMETER_H

#include <QString>
#include "src/core/range.h"
#include <QVector>

#include "mpParser.h"
#include "mpValue.h"
#include "mpVariable.h"

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
    Parameter(QString name, Range range, qreal nominal, QString exp);

    /// Uncertain parameter without reparametrisation.
    Parameter(QString name, Range range, qreal nominal);

    Parameter(Range range);

    Parameter();

    /// Constant, named by its textual value.
    Parameter (qreal value);

    /// Named constant.
    Parameter (QString name, qreal value);

    void setName(QString name);

    void setRange (Range range);

    void setNominal(qreal nominal);

    /// True for uncertain parameters, false for constants.
    bool isUncertain ();

    void setUncertain (bool a);

    QString name();

    /// Range with the reparametrisation applied.
    Range range();

    /// Raw range, without the reparametrisation.
    Range rawRange();

    /// Nominal value with the reparametrisation applied.
    qreal nominal();

    /// Raw nominal value, without the reparametrisation.
    qreal rawNominal();

    QString expression();

private:
    /// The reparametrisation applied to one value, parsed once per thread.
    qreal realValueOf(qreal value) const;

    //Initialised here, not constructor by constructor: the value
    //constructors used to leave m_hasExpression indeterminate, and reading
    //it (the copy constructor does) is undefined behaviour. It stayed
    //harmless only because range() and nominal() return early on
    //!m_uncertain, so a single setUncertain(true) - or swapping those two
    //checks - would have turned a stack byte into a muParserX evaluation of
    //an undefined variable.
    QString m_name;
    Range m_range;
    qreal m_nominal = 0.0;
    bool m_uncertain = false;
    QString m_expression;
    bool m_hasExpression = false;

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::Parameter;

#endif // QFTBX_PARAMETER_H
