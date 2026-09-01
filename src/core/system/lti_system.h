#ifndef QFTBX_LTI_SYSTEM_H
#define QFTBX_LTI_SYSTEM_H

#include <complex>
#include <memory>
#include <vector>

#include <QString>

#include "src/core/system/parameter.h"
#include "mpParser.h"

namespace qftbx {

/**
 * @brief Abstract base for every LTI system handled by the toolbox.
 *
 * Both the plant and the controller structure are represented through this
 * hierarchy. A system is defined by a numerator, a denominator, a gain and a
 * pure delay, each of which may be an uncertain Parameter; concrete
 * subclasses fix the mathematical form (see SystemType). A system HOLDS its
 * parameters by value: there is no ownership to transfer, disarm or share
 * (the historical interface handed over raw pointers and offered
 * releaseOwnership() to disarm deletion, which every consumer had to get
 * right by hand).
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
     * type from parameter VALUES (copied or moved in, no ownership to hand
     * over). The expression strings are only used by FreeForm.
     *
     * The new system belongs to the caller, and the type says so: the
     * historical factory returned a raw pointer, and every one of its
     * callers had to remember a delete on every path out.
     */
    virtual std::unique_ptr<LtiSystem> create (QString name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(qreal(0)),
                              QString numeratorExpr = QString(), QString denominatorExpr = QString()) = 0;

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

    /**
     * @brief The transfer function at s = j*omega, computed DIRECTLY in
     * complex arithmetic from the coefficient VALUES given, in the order of
     * the parameter vectors.
     *
     * A name appearing more than once is ONE variable: every appearance must
     * be given the same value.
     */
    virtual std::complex <qreal> valueAt(qreal w, const std::vector<qreal> & numerator,
                                         const std::vector<qreal> & denominator,
                                         qreal gain, qreal delay) = 0;

    virtual std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) = 0;

    /// The system's own parameters, by reference (it holds them by value).
    virtual std::vector <Parameter> & denominator() = 0;

    /// The system's own parameters, by reference (it holds them by value).
    virtual std::vector <Parameter> & numerator() = 0;

    /// Free-text numerator; empty except for FreeForm.
    virtual QString numeratorString() = 0;

    /// Free-text denominator; empty except for FreeForm.
    virtual QString denominatorString() = 0;

    virtual Parameter & gain () = 0;

    virtual Parameter & delay() = 0;

    /**
     * @brief Mathematical form of the system. The numeric values are
     * serialised in .qft files: do not reorder.
     */
    enum class SystemType {FreeForm, ZeroPoleGain, TimeConstantGain, PolynomialForm};

    virtual SystemType type () = 0;

    /// Copy of the whole system, of the same dynamic type, for the caller.
    virtual std::unique_ptr<LtiSystem> clone () = 0;

private:
    QString m_name;
};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::LtiSystem;

#endif // QFTBX_LTI_SYSTEM_H
