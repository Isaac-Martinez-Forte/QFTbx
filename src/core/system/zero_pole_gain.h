#ifndef QFTBX_ZERO_POLE_GAIN_H
#define QFTBX_ZERO_POLE_GAIN_H

#include "transfer_function.h"
#include "mpParser.h"

#include <QString>
#include <QDebug>

namespace qftbx {

/**
 * @brief Transfer function in zero-pole-gain form:
 * \f$ P(s) = k \, e^{-s\tau} \prod_i (s + z_i) / \prod_j (s + p_j) \f$.
 *
 * Each numerator/denominator Parameter is a root (sign changed); an empty
 * vector stands for the constant 1.
 */
class ZeroPoleGain : public TransferFunction
{

public:
    ZeroPoleGain(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    LtiSystem * create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(qreal(0)), QString numeratorExpr = QString(), QString denominatorExpr = QString());

    ~ZeroPoleGain();

    QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega);

    QString expression(qreal w);

    QString expression();

    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega);

    SystemType type();

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::ZeroPoleGain;

#endif // QFTBX_ZERO_POLE_GAIN_H
