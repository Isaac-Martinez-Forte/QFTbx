/**
 * @file
 * @brief Transfer function as a quotient of polynomials.
 *
 * Declares the form
 * \f$ P(s) = k \, e^{-s\tau} (a_0 s^{n-1} + \dots + a_{n-1}) / (b_0 s^{m-1} + \dots + b_{m-1}) \f$,
 * whose numerator and denominator are coefficient vectors from highest to
 * lowest degree. An empty vector stands for the constant 1, and the poles
 * are the roots of the denominator.
 */

#ifndef QFTBX_POLYNOMIAL_FORM_H
#define QFTBX_POLYNOMIAL_FORM_H

#include "src/core/system/transfer_function.h"

#include <string>

namespace qftbx {

class PolynomialForm : public TransferFunction
{
public:
    PolynomialForm(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) override;

    std::string expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;
    std::optional<std::vector<std::complex<double>>> polesAt(const std::vector<double> & numerator,
                                                             const std::vector<double> & denominator) override;
    std::optional<Polynomials> polynomialsAt(const std::vector<double> & numerator,
                                             const std::vector<double> & denominator,
                                             double gain) override;

    SystemType type() override;

};

}

#endif
