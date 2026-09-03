#ifndef QFTBX_FREE_FORM_H
#define QFTBX_FREE_FORM_H

#include <vector>

#include "transfer_function.h"
#include "complex"
#include "mpParser.h"

namespace qftbx {

/**
 * @brief Transfer function defined by free-text expressions in 's':
 * \f$ P(s) = k \, e^{-s\tau} \, N(s) / D(s) \f$.
 *
 * The parameter vectors do not describe the structure (numerator and
 * denominator are text): they only enumerate the uncertain parameters
 * present in the expressions, for the template sweep. Evaluation replaces
 * 's' textually and hands the expression to muParserX.
 */

class FreeForm : public TransferFunction
{
public:

    /// The parameter vectors list the uncertain parameters appearing in the
    /// numerator/denominator expression texts.
    FreeForm(QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator, Parameter k, Parameter delay, QString numeratorExpr,
                 QString denominatorExpr);

    QString expression (std::vector <double> * numerator, std::vector <double> * denominator,
                             double k, double delay, double omega) override;

    QString expression(double w) override;

    QString expression() override;

    std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                 const std::vector<double> & denominator,
                                 double gain, double delay) override;

    std::complex <double> evaluateNumerator(std::vector <double> * nume, double omega) override;

    std::complex <double> evaluateDenominator(std::vector <double> * deno, double omega) override;

    std::complex <double> evaluate (std::vector <double> * numerator, std::vector <double> * denominator,
                                           double k, double delay, double omega) override;

    //Re-expose the inherited nominal evaluation hidden by the overloads above.
    using TransferFunction::evaluate;

    SystemType type() override;

    std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)), QString numeratorExpr = QString(), QString denominatorExpr = QString()) override;

    QString numeratorString() override;
    QString denominatorString() override;

    std::unique_ptr<LtiSystem> clone() override;

private:
    QString m_numeratorExpr;
    QString m_denominatorExpr;

    /// The two above with the Laplace variable bound. Built in the
    /// constructor and const thereafter: valueAt() runs on one plant from
    /// several threads, so anything it fills in lazily is a data race.
    QString m_boundExpression;

    static const QString & laplaceName();
};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::FreeForm;

#endif // QFTBX_FREE_FORM_H
