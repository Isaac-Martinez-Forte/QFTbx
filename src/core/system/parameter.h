#ifndef QFTBX_PARAMETER_H
#define QFTBX_PARAMETER_H

#include <QString>
#include <QPointF>
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
    Parameter(QString name, QPointF range, qreal nominal, QString exp);

    /// Uncertain parameter without reparametrisation.
    Parameter(QString name, QPointF range, qreal nominal);

    Parameter(QPointF range);

    Parameter (const Parameter &obj);

    Parameter();

    Parameter * clone ();

    /// Deep copy of a parameter vector; the caller owns the copy.
    static QVector <Parameter*> * cloneVector(QVector <Parameter*> * source);

    /// Constant, named by its textual value.
    Parameter (qreal value);

    /// Named constant.
    Parameter (QString name, qreal value);

    void setName(QString name);

    void setRange (QPointF range);

    void setNominal(qreal nominal);

    /// True for uncertain parameters, false for constants.
    bool isUncertain ();

    void setUncertain (bool a);

    QString name();

    /// Range with the reparametrisation applied.
    QPointF range();

    /// Raw range, without the reparametrisation.
    QPointF rawRange();

    /// Nominal value with the reparametrisation applied.
    qreal nominal();

    /// Raw nominal value, without the reparametrisation.
    qreal rawNominal();

    QString expression();

private:
    QString m_name;
    QPointF m_range;
    qreal m_nominal;
    bool m_uncertain;
    QString m_expression;
    bool m_hasExpression;

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::Parameter;

#endif // QFTBX_PARAMETER_H
