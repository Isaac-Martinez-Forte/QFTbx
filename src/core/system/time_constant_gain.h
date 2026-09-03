#ifndef QFTBX_TIME_CONSTANT_GAIN_H
#define QFTBX_TIME_CONSTANT_GAIN_H

#include <QString>

#include "transfer_function.h"
#include "mpParser.h"

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
    TimeConstantGain(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay);

    std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), QString numeratorExpr = QString(), QString denominatorExpr = QString()) override;

    ~TimeConstantGain();

    SystemType type() override;

    QString expression (std::vector <double> * numerator, std::vector <double> * denominator,
                             double k, double delay, double omega) override;

    QString expression(double w) override;

    QString expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;

    std::complex <double> evaluateNumerator(std::vector <double> * nume, double omega) override;

    std::complex <double> evaluateDenominator(std::vector <double> * deno, double omega) override;

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::TimeConstantGain;

#endif // QFTBX_TIME_CONSTANT_GAIN_H
