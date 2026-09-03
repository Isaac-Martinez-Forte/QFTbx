#ifndef QFTBX_POLYNOMIAL_FORM_H
#define QFTBX_POLYNOMIAL_FORM_H

#include "transfer_function.h"

#include <string>

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
    PolynomialForm(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) override;

    ~PolynomialForm();

    std::string expression (std::vector <double> * numerator, std::vector <double> * denominator,
                             double k, double delay, double omega) override;

    std::string expression(double w) override;

    std::string expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;

    std::complex <double> evaluateNumerator(std::vector <double> * nume, double omega) override;

    std::complex <double> evaluateDenominator(std::vector <double> * deno, double omega) override;

    SystemType type() override;

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::PolynomialForm;

#endif // QFTBX_POLYNOMIAL_FORM_H
