#ifndef QFTBX_FREE_FORM_H
#define QFTBX_FREE_FORM_H

#include <QVector>

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

    QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega) override;

    QString expression(qreal w) override;

    QString expression() override;

    std::complex <qreal> valueAt(qreal w, const std::vector<qreal> & numerator,
                                 const std::vector<qreal> & denominator,
                                 qreal gain, qreal delay) override;

    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) override;

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) override;

    std::complex <qreal> evaluate (QVector <qreal> * numerator, QVector <qreal> * denominator,
                                           qreal k, qreal delay, qreal omega) override;

    //Re-expose the inherited nominal evaluation hidden by the overloads above.
    using TransferFunction::evaluate;

    SystemType type() override;

    std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(qreal(0)), QString numeratorExpr = QString(), QString denominatorExpr = QString()) override;

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
