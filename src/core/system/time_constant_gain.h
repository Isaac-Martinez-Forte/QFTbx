#ifndef QFTBX_TIME_CONSTANT_GAIN_H
#define QFTBX_TIME_CONSTANT_GAIN_H

#include <QString>

#include "transfer_function.h"
#include "mpParser.h"

namespace qftbx {

/**
 * @brief Transfer function in time-constant (DC-gain) form:
 * \f$ P(s) = k \, e^{-s\tau} \prod_i (s/z_i + 1) / \prod_j (s/p_j + 1) \f$.
 *
 * Each numerator/denominator Parameter is a corner frequency; at s = 0 the
 * system equals k. An empty vector stands for the constant 1.
 */
class TimeConstantGain : public TransferFunction
{
public:
    TimeConstantGain(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(qreal(0)), QString numeratorExpr = QString(), QString denominatorExpr = QString());

    ~TimeConstantGain();

    SystemType type();

    QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega);

    QString expression(qreal w);

    QString expression();

    std::complex <qreal> valueAt(qreal w, const std::vector<qreal> & numerator,
                                 const std::vector<qreal> & denominator,
                                 qreal gain, qreal delay) override;

    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega);

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TimeConstantGain;

#endif // QFTBX_TIME_CONSTANT_GAIN_H
