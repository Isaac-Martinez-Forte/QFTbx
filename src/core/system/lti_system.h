#ifndef QFTBX_LTI_SYSTEM_H
#define QFTBX_LTI_SYSTEM_H

#include <complex>
#include <optional>
#include <memory>
#include <vector>

#include <string>

#include "src/core/system/parameter.h"

namespace qftbx {

/**
 * @brief Abstract base for every LTI system handled by the toolbox.
 *
 * Both the plant and the controller structure are represented through this
 * hierarchy. A system is defined by a numerator, a denominator, a gain and a
 * pure delay, each of which may be an uncertain Parameter; concrete
 * subclasses fix the mathematical form (see SystemType). A system HOLDS its
 * parameters by value: there is no ownership to transfer, disarm or share.
 *
 * Evaluation is direct complex arithmetic over the coefficient values
 * (valueAt); only a free-form system evaluates an expression tree, with its
 * coefficients bound as variables.
 */
class LtiSystem
{
public:
    LtiSystem(std::string name);

    /**
     * @brief Virtual constructor: builds a new instance of the same dynamic
     * type from parameter VALUES (copied or moved in, no ownership to hand
     * over). The expression strings are only used by FreeForm.
     *
     * The new system belongs to the caller, and the type says so.
     */
    virtual std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)),
                              std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) = 0;

    virtual ~LtiSystem() {}

    void setName (std::string name);

    const std::string & name() const;

    /**
     * @brief What the plant IS, in the user's words: where it comes from,
     * what it models, why its uncertainty is what it is.
     *
     * A name fits in a list and nothing more, and a plant that is worth
     * saving is worth a line saying what it was. Free text, saved with the
     * project, and deliberately NOT part of sameAs(): rewriting the
     * description of a plant does not change the plant, and throwing away
     * an hour of templates over a typo in a comment would be absurd.
     */
    void setDescription(std::string description);

    const std::string & description() const;

    /// Value of the system at s = j*omega using the nominal parameter values.
    virtual std::complex <double> evaluate (double omega) = 0;

    /// One value per frequency.
    virtual std::vector <std::complex <double> > evaluate (const std::vector <double> & omega) = 0;

    /// Symbolic expression in 's', for display.
    virtual std::string expression() = 0;

    /**
     * @brief The transfer function at s = j*omega, computed DIRECTLY in
     * complex arithmetic from the coefficient VALUES given, in the order of
     * the parameter vectors.
     *
     * A name appearing more than once is ONE variable: every appearance must
     * be given the same value.
     */
    virtual std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                         const std::vector<double> & denominator,
                                         double gain, double delay) = 0;

    /**
     * @brief The poles of the system for the coefficient values given, in the
     * order of the parameter vectors, or nothing when they cannot be known.
     *
     * What the stability criterion needs of a plant, once: how many poles lie
     * in the right half-plane and where the ones on the imaginary axis are.
     * The forms that name their poles answer directly; the polynomial form
     * finds the roots of its denominator; a free-form plant recovers the
     * polynomial its denominator expression evaluates to, and answers
     * nothing when that expression is not a polynomial in s (a delay written
     * into it, a transcendental), which the criterion then refuses to guess.
     */
    virtual std::optional<std::vector<std::complex<double>>> polesAt(const std::vector<double> & numerator,
                                                                     const std::vector<double> & denominator) = 0;

    /// polesAt() at the nominal values of the parameters.
    virtual std::optional<std::vector<std::complex<double>>> nominalPoles() = 0;

    /// The system's own parameters, by reference (it holds them by value).
    virtual std::vector <Parameter> & denominator() = 0;

    /// The system's own parameters, by reference (it holds them by value).
    virtual std::vector <Parameter> & numerator() = 0;

    /// Free-text numerator; empty except for FreeForm.
    virtual std::string numeratorString() = 0;

    /// Free-text denominator; empty except for FreeForm.
    virtual std::string denominatorString() = 0;

    virtual Parameter & gain () = 0;

    virtual Parameter & delay() = 0;

    /**
     * @brief Mathematical form of the system. The numeric values are
     * serialised in .qft files: do not reorder.
     */
    enum class SystemType {FreeForm, ZeroPoleGain, TimeConstantGain, PolynomialForm};

    virtual SystemType type () = 0;

    /**
     * @brief Value equality between two systems, for telling a real change
     * from a dialog accepted without an edit.
     *
     * Not virtual, so it CANNOT be overridden: it compares the dynamic TYPE
     * and then everything a system is made of - name, the textual numerator
     * and denominator, and the four parameter groups. A family with state
     * beyond the parameters has one way to take part, which is to expose that
     * state through numeratorString() and denominatorString(): that is how a
     * FreeForm's two expressions get compared.
     *
     * Conservative by design: the two answers are not symmetric. A wrong
     * "equal" keeps the templates computed for the OLD plant, which is the
     * silent defect this class's own users warn about; a wrong "different"
     * costs one recomputation. Nothing is left out on the grounds that it
     * probably does not matter.
     */
    bool sameAs(LtiSystem & other);

    /// Copy of the whole system, of the same dynamic type, for the caller.
    virtual std::unique_ptr<LtiSystem> clone () = 0;

private:
    std::string m_name;
    std::string m_description;
};

} // namespace qftbx


#endif // QFTBX_LTI_SYSTEM_H
