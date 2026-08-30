#ifndef QFTBX_POLYNOMIAL_FORM_H
#define QFTBX_POLYNOMIAL_FORM_H

#include "transfer_function.h"

#include <QString>

#include "mpParser.h"

namespace qftbx {

/**
 * @brief Transfer function as a quotient of polynomials given by their
 * coefficients:
 * \f$ P(s) = k \, e^{-s\tau} (a_0 s^{n-1} + \dots + a_{n-1}) /
 *                          (b_0 s^{m-1} + \dots + b_{m-1}) \f$.
 *
 * Coefficient vectors run from highest to lowest degree; the degree of the
 * i-th entry is size - 1 - i. An empty vector stands for the constant 1.
 */
class PolynomialForm : public TransferFunction
{
public:
    PolynomialForm(QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator, Parameter * k, Parameter* delay);

    LtiSystem * create (QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator,
                              Parameter * k, Parameter* delay, QString numeratorExpr = 0, QString denominatorExpr = 0);

    ~PolynomialForm();

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
using qftbx::PolynomialForm;

#endif // QFTBX_POLYNOMIAL_FORM_H
