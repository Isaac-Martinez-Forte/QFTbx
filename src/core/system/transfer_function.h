#ifndef QFTBX_TRANSFER_FUNCTION_H
#define QFTBX_TRANSFER_FUNCTION_H

#include "lti_system.h"
#include <QVector>
#include "src/core/system/parameter.h"
#include "mpParser.h"

namespace qftbx {

/**
 * @brief Common implementation for transfer-function systems.
 *
 * Owns the numerator/denominator Parameter vectors, the gain and the delay
 * (whoever constructs a system hands over ownership; releaseOwnership()
 * disarms deletion for structures that share the pointers). Subclasses only
 * provide the expression generators for their mathematical form.
 */
class TransferFunction : public LtiSystem
{
public:
    TransferFunction(QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator, Parameter * k, Parameter* delay);

    virtual LtiSystem * create (QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator,
                              Parameter * k, Parameter* delay = NULL, QString numeratorExpr = 0, QString denominatorExpr = 0) = 0;

    ~TransferFunction();

    std::complex <qreal> evaluate (qreal omega);

    QVector <std::complex <qreal> > * evaluate (QVector <qreal> * omega);

    std::complex <qreal> evaluate (QVector <qreal> * numerator, QVector <qreal> * denominator,
                                           qreal k, qreal delay, qreal omega);

    virtual QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega) = 0;

    virtual QString expression(qreal w) = 0;

    virtual QString expression() = 0;

    virtual std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) = 0;

    QVector <Parameter*> * numerator();

    void releaseOwnership ();

    QVector <Parameter*> * denominator();

    QString numeratorString();

    QString denominatorString();

    Parameter * gain();

    Parameter * delay();

    virtual SystemType type() = 0;

    LtiSystem * clone ();

protected:
    Parameter * m_gain;
    Parameter * m_delay;

    QVector <Parameter*> * m_numerator;
    QVector <Parameter*> * m_denominator;

    bool m_ownsData = true;

};


} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TransferFunction;

#endif // QFTBX_TRANSFER_FUNCTION_H
