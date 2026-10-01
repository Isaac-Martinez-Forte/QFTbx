/**
 * @file
 * @brief Abstract base of every LTI system the toolbox handles, and the
 * nominal values, polynomials and closed loops taken from one.
 *
 * Both the plant and the controller structure are systems: a numerator, a
 * denominator, a gain and a pure delay, each a possibly uncertain parameter
 * held by value, so there is no ownership to transfer or share, and the
 * concrete subclasses fix the mathematical form (SystemType, whose values are
 * written to .qft files and must keep their order). create is the virtual
 * constructor, building a system of the same type from parameter values, and
 * clone copies a whole system; both hand the new one to the caller. A system
 * evaluates at s = j omega from its nominal values (evaluate) or from
 * coefficient values given in the order of its parameter vectors (valueAt),
 * in direct complex arithmetic, only a free form evaluating an expression
 * tree; a name that appears more than once is one variable and takes one
 * value. polesAt names the poles for given values, which is what the
 * stability criterion asks of a plant, directly for the forms that name them,
 * from the roots of the denominator for the polynomial form, and from the
 * polynomial a free-form denominator evaluates to, answering nothing when it
 * is not a polynomial in s; nominalPoles is polesAt at the nominal values.
 * polynomialsAt gives the numerator and the denominator as real polynomials,
 * highest degree first, the gain folded into the numerator and the delay left
 * out, or nothing when either is not a polynomial.
 *
 * The description says what a plant is in the user's words; it is saved with
 * the project and left out of sameAs, the value equality that tells a real
 * change from a dialog accepted without an edit. sameAs is not virtual: it
 * compares the dynamic type and then everything a system is made of, its name,
 * the textual numerator and denominator, through which a free form takes part
 * with its expressions, and the four parameter groups. It errs on the side of
 * "different", because a wrong "equal" keeps the templates computed for
 * another plant while a wrong "different" costs one recomputation.
 *
 * nominalValues takes the nominal of each parameter, nominalPolynomials the
 * polynomials of a system at its nominal values, hasDelay says whether a
 * system has a delay, and characteristicOf closes a loop, N_P N_C + D_P D_C,
 * summing the two products once they are formed.
 */

#ifndef QFTBX_LTI_SYSTEM_H
#define QFTBX_LTI_SYSTEM_H

#include <complex>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "src/core/system/parameter.h"

namespace qftbx {

class LtiSystem
{
public:
    LtiSystem(std::string name);

    virtual std::unique_ptr<LtiSystem> create (std::string name, std::vector <Parameter> numerator, std::vector <Parameter> denominator,
                              Parameter k, Parameter delay = Parameter(double(0)),
                              std::string numeratorExpr = std::string(), std::string denominatorExpr = std::string()) = 0;

    virtual ~LtiSystem() {}

    void setName (std::string name);

    const std::string & name() const;

    void setDescription(std::string description);

    const std::string & description() const;

    virtual std::complex <double> evaluate (double omega) = 0;

    virtual std::vector <std::complex <double> > evaluate (const std::vector <double> & omega) = 0;

    virtual std::string expression() = 0;

    virtual std::complex <double> valueAt(double w, const std::vector<double> & numerator,
                                         const std::vector<double> & denominator,
                                         double gain, double delay) = 0;

    virtual std::optional<std::vector<std::complex<double>>> polesAt(const std::vector<double> & numerator,
                                                                     const std::vector<double> & denominator) = 0;

    virtual std::optional<std::vector<std::complex<double>>> nominalPoles() = 0;

    struct Polynomials {
        std::vector<double> numerator;
        std::vector<double> denominator;
    };
    virtual std::optional<Polynomials> polynomialsAt(const std::vector<double> & numerator,
                                                     const std::vector<double> & denominator,
                                                     double gain) = 0;

    virtual std::vector <Parameter> & denominator() = 0;

    virtual std::vector <Parameter> & numerator() = 0;

    virtual std::string numeratorString() = 0;

    virtual std::string denominatorString() = 0;

    virtual Parameter & gain () = 0;

    virtual Parameter & delay() = 0;

    enum class SystemType {FreeForm, ZeroPoleGain, TimeConstantGain, PolynomialForm};

    virtual SystemType type () = 0;

    bool sameAs(LtiSystem & other);

    virtual std::unique_ptr<LtiSystem> clone () = 0;

private:
    std::string m_name;
    std::string m_description;
};

std::vector<double> nominalValues(const std::vector<Parameter> & parameters);

bool hasDelay(LtiSystem & system);

std::optional<LtiSystem::Polynomials> nominalPolynomials(LtiSystem & system);

std::vector<double> characteristicOf(const LtiSystem::Polynomials & plant, const LtiSystem::Polynomials & loop);

}

#endif
