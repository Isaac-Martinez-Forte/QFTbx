#ifndef QFTBX_LTI_SYSTEM_H
#define QFTBX_LTI_SYSTEM_H

#include <QString>
#include <complex>

#include "src/core/system/parameter.h"
#include "mpParser.h"

namespace qftbx {

/**
 * @brief Abstract base for every LTI system handled by the toolbox.
 *
 * Both the plant and the controller structure are represented through this
 * hierarchy. A system is defined by a numerator, a denominator, a gain and a
 * pure delay, each of which may be an uncertain Parameter; concrete
 * subclasses fix the mathematical form (see SystemType).
 *
 * Evaluation currently works by generating a textual expression that is
 * evaluated with muParserX; expression(w) keeps uncertain parameters by name
 * so the template sweep can drive them through the parser's symbol table.
 */
class LtiSystem
{
public:
    LtiSystem(QString name);

    /**
     * @brief Virtual constructor: builds a new instance of the same dynamic
     * type, taking ownership of every pointer. A null delay means no delay.
     * The expression strings are only used by FreeForm.
     */
    virtual LtiSystem * create (QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator,
                              Parameter * k, Parameter* delay = NULL, QString numeratorExpr = 0, QString denominatorExpr = 0) = 0;

    virtual ~LtiSystem() {}

    void setName (QString name);

    QString name();

    /// Value of the system at s = j*omega using the nominal parameter values.
    virtual std::complex <qreal> evaluate (qreal omega) = 0;

    /// One value per frequency; the caller owns the returned vector.
    virtual QVector <std::complex <qreal> > * evaluate (QVector <qreal> * omega) = 0;

    /// Value at s = j*omega for explicit numeric parameter values.
    virtual std::complex <qreal> evaluate (QVector <qreal> * numerator, QVector <qreal> * denominator,
                                           qreal k, qreal delay, qreal omega) = 0;

    /// Expression for explicit numeric parameter values at s = j*omega.
    virtual QString expression (QVector <qreal> * numerator, QVector <qreal> * denominator,
                             qreal k, qreal delay, qreal omega) = 0;

    /// Expression at s = j*omega; uncertain parameters stay by name.
    virtual QString expression(qreal w) = 0;

    /// Symbolic expression in 's', for display.
    virtual QString expression() = 0;

    virtual std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) = 0;

    /// Internal vector: not a copy, the system keeps ownership.
    virtual QVector <Parameter*> * denominator() = 0;

    /// Internal vector: not a copy, the system keeps ownership.
    virtual QVector <Parameter*> * numerator() = 0;

    /// Free-text numerator; empty except for FreeForm.
    virtual QString numeratorString() = 0;

    /// Free-text denominator; empty except for FreeForm.
    virtual QString denominatorString() = 0;

    virtual Parameter * gain () = 0;

    virtual Parameter * delay() = 0;

    /**
     * @brief Mathematical form of the system. The numeric values are
     * serialised in .qft files: do not reorder.
     */
    enum class SystemType {FreeForm, ZeroPoleGain, TimeConstantGain, PolynomialForm};

    virtual SystemType type () = 0;

    /// Gives up ownership of parameters and vectors (shared-pointer case).
    virtual void releaseOwnership () = 0;

    /// Deep copy: the clone owns fresh copies of every Parameter.
    virtual LtiSystem * clone () = 0;

private:
    QString m_name;
};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::LtiSystem;

#endif // QFTBX_LTI_SYSTEM_H
