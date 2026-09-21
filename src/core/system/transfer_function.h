/**
 * @file
 * @brief Common implementation of the transfer-function systems.
 *
 * Declares the intermediate class that holds the numerator and denominator
 * parameters, the gain and the delay by value and answers the nominal
 * evaluation, the nominal poles and cloning for every form; the concrete
 * forms only provide their expression and the evaluation of their shape
 * from coefficient values.
 */

#ifndef QFTBX_TRANSFER_FUNCTION_H
#define QFTBX_TRANSFER_FUNCTION_H

#include <string>
#include <vector>

#include "src/core/system/lti_system.h"
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
    std::optional<std::vector<std::complex<double>>> nominalPoles() override;

    std::string expression() override = 0;

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

}

#endif
