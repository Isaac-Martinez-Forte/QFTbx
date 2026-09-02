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
                              Parameter k, Parameter delay = Parameter(double(0)),
                              QString numeratorExpr = QString(), QString denominatorExpr = QString()) override = 0;

    std::complex <double> evaluate (double omega) override;

    QVector <std::complex <double> > evaluate (const QVector <double> & omega) override;

    std::complex <double> evaluate (QVector <double> * numerator, QVector <double> * denominator,
                                           double k, double delay, double omega) override;

    QString expression (QVector <double> * numerator, QVector <double> * denominator,
                             double k, double delay, double omega) override = 0;

    QString expression(double w) override = 0;

    QString expression() override = 0;

    std::complex <double> evaluateNumerator(QVector <double> * nume, double omega) override = 0;

    std::complex <double> evaluateDenominator(QVector <double> * deno, double omega) override = 0;

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
