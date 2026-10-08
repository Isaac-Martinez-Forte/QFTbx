/**
 * @file
 * @brief Abstract base of every LTI system the toolbox handles, and the
 * nominal values, polynomials and closed loops taken from one.
 *
 * The plant and the controller structure are both systems: a numerator, a
 * denominator, a gain and a pure delay, each a possibly uncertain parameter
 * held by value, with the mathematical form fixed by the concrete subclass
 * (SystemType, whose values are written to .qft files and keep their
 * order). create and clone build a new system for the caller. evaluate and
 * valueAt evaluate at s = j omega from the nominal values or from given
 * coefficients; polesAt and nominalPoles name the poles; polynomialsAt
 * gives the numerator and denominator as real polynomials, highest degree
 * first, with the gain in the numerator and the delay left out, or nothing
 * when they are not polynomials.
 *
 * sameAs is the value equality that tells a real change from a dialog
 * accepted without an edit: the dynamic type, the name, the textual
 * numerator and denominator and the four parameter groups, but not the
 * description. It errs on the side of different, since a wrong equal would
 * keep the templates of another plant.
 *
 * nominalValues, nominalPolynomials and hasDelay read a system, and
 * characteristicOf closes a loop, N_P N_C + D_P D_C.
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
