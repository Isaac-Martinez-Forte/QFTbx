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
    PolynomialForm(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), QString numeratorExpr = QString(), QString denominatorExpr = QString()) override;

    ~PolynomialForm();

    QString expression (QVector <double> * numerator, QVector <double> * denominator,
                             double k, double delay, double omega) override;

    QString expression(double w) override;

    QString expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;

    std::complex <double> evaluateNumerator(QVector <double> * nume, double omega) override;

    std::complex <double> evaluateDenominator(QVector <double> * deno, double omega) override;

    SystemType type() override;

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::PolynomialForm;

#endif // QFTBX_POLYNOMIAL_FORM_H
