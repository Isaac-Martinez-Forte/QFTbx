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

    std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(qreal(0)),
                              QString numeratorExpr = QString(), QString denominatorExpr = QString()) override = 0;

    std::complex <qreal> evaluate (qreal omega) override;

    QVector <std::complex <qreal> > evaluate (const QVector <qreal> & omega) override;

    std::complex <qreal> evaluate (QVector <qreal> * numerator, QVector <qreal> * denominator,
                                           qreal k, qreal delay, qreal omega) override;

    QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega) override = 0;

    QString expression(qreal w) override = 0;

    QString expression() override = 0;

    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) override = 0;

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) override = 0;

    std::vector <Parameter> & numerator() override;

    std::vector <Parameter> & denominator() override;

    QString numeratorString() override;

    QString denominatorString() override;

    Parameter & gain() override;

    Parameter & delay() override;

    SystemType type() override = 0;

    std::unique_ptr<LtiSystem> clone () override;

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
