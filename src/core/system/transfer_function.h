#ifndef QFTBX_TRANSFER_FUNCTION_H
#define QFTBX_TRANSFER_FUNCTION_H

#include <string>
#include <vector>

#include "lti_system.h"
#include "src/core/system/parameter.h"

namespace qftbx {

/**
 * @brief Common implementation for transfer-function systems.
 *
 * Holds the numerator/denominator parameters, the gain and the delay BY
 * VALUE. Subclasses only provide the expression generators for their
 * mathematical form.
 */
class TransferFunction : public LtiSystem
{
public:
    TransferFunction(std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                     Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)),
                              std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) override = 0;

    std::complex <double> evaluate (double omega) override;

    std::vector <std::complex <double> > evaluate (const std::vector <double> & omega) override;

    std::complex <double> evaluate (std::vector <double> * numerator, std::vector <double> * denominator,
                                           double k, double delay, double omega) override;

    std::string expression (std::vector <double> * numerator, std::vector <double> * denominator,
                             double k, double delay, double omega) override = 0;

    std::string expression(double w) override = 0;

    std::string expression() override = 0;

    std::complex <double> evaluateNumerator(std::vector <double> * nume, double omega) override = 0;

    std::complex <double> evaluateDenominator(std::vector <double> * deno, double omega) override = 0;

    std::vector <Parameter> & numerator() override;

    std::vector <Parameter> & denominator() override;

    std::string numeratorString() override;

    std::string denominatorString() override;

    Parameter & gain() override;

    Parameter & delay() override;

    SystemType type() override = 0;

    std::unique_ptr<LtiSystem> clone () override;

protected:
    Parameter m_gain;
    Parameter m_delay;

    std::vector <Parameter> m_numerator;
    std::vector <Parameter> m_denominator;
};


} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TransferFunction;

#endif // QFTBX_TRANSFER_FUNCTION_H
