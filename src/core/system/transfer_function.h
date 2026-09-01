#ifndef QFTBX_TRANSFER_FUNCTION_H
#define QFTBX_TRANSFER_FUNCTION_H

#include <vector>

#include "lti_system.h"
#include <QVector>
#include "src/core/system/parameter.h"
#include "mpParser.h"

namespace qftbx {

/**
 * @brief Common implementation for transfer-function systems.
 *
 * Holds the numerator/denominator parameters, the gain and the delay BY
 * VALUE. Subclasses only provide the expression generators for their
 * mathematical form.
 */
class TransferFunction : public LtiSystem
{
public:
    TransferFunction(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                     Parameter k, Parameter delay);

    virtual std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(qreal(0)),
                              QString numeratorExpr = QString(), QString denominatorExpr = QString()) = 0;

    std::complex <qreal> evaluate (qreal omega);

    QVector <std::complex <qreal> > evaluate (const QVector <qreal> & omega);

    std::complex <qreal> evaluate (QVector <qreal> * numerator, QVector <qreal> * denominator,
                                           qreal k, qreal delay, qreal omega);

    virtual QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega) = 0;

    virtual QString expression(qreal w) = 0;

    virtual QString expression() = 0;

    virtual std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) = 0;

    std::vector <Parameter> & numerator();

    std::vector <Parameter> & denominator();

    QString numeratorString();

    QString denominatorString();

    Parameter & gain();

    Parameter & delay();

    virtual SystemType type() = 0;

    std::unique_ptr<LtiSystem> clone ();

protected:
    Parameter m_gain;
    Parameter m_delay;

    std::vector <Parameter> m_numerator;
    std::vector <Parameter> m_denominator;
};


} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TransferFunction;

#endif // QFTBX_TRANSFER_FUNCTION_H
