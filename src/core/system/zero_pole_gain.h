#ifndef QFTBX_ZERO_POLE_GAIN_H
#define QFTBX_ZERO_POLE_GAIN_H

#include "transfer_function.h"

#include <string>

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
    ZeroPoleGain(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) override;

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
using qftbx::ZeroPoleGain;

#endif // QFTBX_ZERO_POLE_GAIN_H
