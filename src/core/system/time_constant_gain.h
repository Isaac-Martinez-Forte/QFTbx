/**
 * @file
 * @brief Transfer function in time-constant form.
 *
 * Declares the form
 * \f$ P(s) = k \, e^{-s\tau} \prod_i (s/z_i + 1) / \prod_j (s/p_j + 1) \f$,
 * whose numerator and denominator parameters are corner frequencies, each
 * a factor s/z + 1, times a gain k that is the value at s = 0 and a pure
 * delay. An empty vector stands for the constant 1. A corner that is zero,
 * or whose uncertainty range contains zero, is refused at construction
 * with InvalidInput: every factor divides by it, and zero is a finite
 * number no other check refuses.
 */

#ifndef QFTBX_TIME_CONSTANT_GAIN_H
#define QFTBX_TIME_CONSTANT_GAIN_H

#include <string>

#include "src/core/system/transfer_function.h"

namespace qftbx {

class TimeConstantGain : public TransferFunction
{
public:
    TimeConstantGain(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) override;

    SystemType type() override;

    std::string expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;
    std::optional<std::vector<std::complex<double>>> polesAt(const std::vector<double> & numerator,
                                                             const std::vector<double> & denominator) override;
    std::optional<Polynomials> polynomialsAt(const std::vector<double> & numerator,
                                             const std::vector<double> & denominator,
                                             double gain) override;

};

}

#endif
