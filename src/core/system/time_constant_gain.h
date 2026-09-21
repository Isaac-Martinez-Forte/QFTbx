/**
 * @file
 * @brief Transfer function in time-constant form.
 *
 * Declares the form whose numerator and denominator parameters are corner
 * frequencies, each a factor s/z + 1, times a gain that is the value at
 * s = 0 and a pure delay. An empty vector stands for the constant 1. A
 * corner that is zero, or whose uncertainty range contains zero, is refused
 * at construction: every factor divides by it, and zero is a finite number
 * no other check refuses.
 */

#ifndef QFTBX_TIME_CONSTANT_GAIN_H
#define QFTBX_TIME_CONSTANT_GAIN_H

#include <string>

#include "src/core/system/transfer_function.h"

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
    /**
     * @brief Builds the system. Throws InvalidInput when any corner frequency
     * is zero or its uncertainty range contains zero: every factor is
     * s/z + 1, so a zero corner divides by zero at every frequency, and 0 is a
     * finite number that no other check refuses.
     */
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

};

}

#endif
