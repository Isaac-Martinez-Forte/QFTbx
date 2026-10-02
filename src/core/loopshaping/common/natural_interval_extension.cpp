/**
 * @file
 * @brief The natural interval extension of the loop over a controller box.
 *
 * Each factor (jw + x) is a horizontal segment of the complex plane read in
 * polar form; magnitudes multiply and phases add, as in section 1.2.5 of the
 * thesis, and an empty factor list stands for the constant one of a pure
 * gain. Only the zero-pole-gain structure is projected: the other structures
 * evaluate differently and are refused with a message. Magnitudes are
 * clamped to the positive finite doubles before the logarithm, and phases
 * are shifted by whole turns onto (-2 pi, 0], the whole branch being the
 * answer whenever the set crosses the cut or rounding puts an end past it;
 * the enclosure of a whole turn is computed once.
 */

#include "src/core/loopshaping/common/natural_interval_extension.h"
#include "src/core/math/constants.h"

#include <cmath>
#include <limits>

#include "src/core/common/exception.h"

namespace qftbx {

namespace {

const double kRadToDeg = 180.0 / qftbx::math::kPi;

Interval parameterInterval(Parameter & parameter)
{
    if (parameter.isUncertain()) {
        return Interval(parameter.range().min, parameter.range().max);
    }
    return Interval(parameter.nominal());
}

Interval factorModulusOfSquare(const Interval & xSquared, const Interval & wSquared)
{
    return sqrt(xSquared + wSquared);
}

Interval factorModulus(const Interval & x, const Interval & wSquared)
{
    return factorModulusOfSquare(sqr(x), wSquared);
}

PolarInterval factor(const Interval & x, double w)
{
    const Interval jw(w);
    return PolarInterval(factorModulus(x, sqr(jw)), ComplexInterval(x, jw).phase());
}

Interval loopModulus(const Interval & gain, const Interval & numerator, const Interval & nominal,
                     const Interval & denominator)
{
    const Interval scaled = numerator * nominal;
    const Interval gained = gain.lower() >= 0.0 ? gain * scaled
                          : gain.upper() <= 0.0 ? -gain * scaled
                                                : abs(gain) * scaled;
    return gained / denominator;
}

Interval loopPhase(const Interval & gain, const Interval & numerator, const Interval & nominal,
                   const Interval & denominator)
{
    const Interval phase = numerator + nominal;
    const Interval gained = gain.lower() >= 0.0 ? phase
                          : gain.upper() <= 0.0 ? phase + Interval::pi()
                                                : Interval::hull(phase, phase + Interval::pi());
    return gained - denominator;
}

Interval clampedModulus(const Interval & modulus)
{
    double low = modulus.lower();
    double high = modulus.upper();

    if (high > std::numeric_limits<double>::max()) {
        high = std::numeric_limits<double>::max();
    }
    if (high <= 0.0) {
        high = std::numeric_limits<double>::min();
    }
    if (low <= 0.0) {
        low = std::numeric_limits<double>::min();
    }
    return Interval(low, high);
}

Interval decibelsOf(const Interval & modulus)
{
    return Interval(20.0) * log10(clampedModulus(modulus));
}

Interval onTheBranch(Interval theta)
{
    static const Interval twoPi = Interval(2.0) * Interval::pi();

    if (theta.width() >= twoPi.lower()) {
        theta = Interval(-twoPi.upper(), 0.0);
    } else {
        const double turns = std::ceil(theta.upper() / twoPi.lower());
        theta = theta - Interval(turns) * twoPi;

        if (theta.upper() > 0.0 || theta.lower() < -twoPi.upper()) {
            theta = Interval(-twoPi.upper(), 0.0);
        }
    }

    return theta * Interval(kRadToDeg);
}

void ensureSupportedStructure(LtiSystem::SystemType type)
{
    if (type != LtiSystem::SystemType::ZeroPoleGain) {
        throw InvalidInput(QFTBX_TR("Core", "Loop shaping supports zero-pole-gain controller "
                           "structures only, for now: a time-constant, "
                           "polynomial or free-form controller structure is "
                           "not supported yet."));
    }
}

}

PolarInterval NaturalIntervalExtension::factorProduct(std::vector<Parameter> & parameters, double w)
{
    PolarInterval product(Interval(1.0), Interval(0.0));

    for (Parameter & parameter : parameters) {
        product = product * factor(parameterInterval(parameter), w);
    }

    return product;
}

PolarInterval NaturalIntervalExtension::factorProduct(const std::vector<double> & values, double w)
{
    PolarInterval product(Interval(1.0), Interval(0.0));

    for (const double value : values) {
        product = product * factor(Interval(value), w);
    }

    return product;
}

NicholsBox NaturalIntervalExtension::toNichols(const PolarInterval & loop)
{
    return {decibelsOf(loop.magnitude()), onTheBranch(loop.phase())};
}

NicholsBox NaturalIntervalExtension::nicholsBox(LtiSystem * controller, double w, std::complex<double> p0)
{
    return nicholsBox(controller, w, p0, parameterInterval(controller->gain()));
}

NicholsBox NaturalIntervalExtension::nicholsBox(LtiSystem * controller, double w, std::complex<double> p0,
                                                const Interval & gain)
{
    return nicholsOf(gain, factorsOf(controller, w), p0);
}

NicholsBox NaturalIntervalExtension::nicholsPoint(const PointController & point, double w,
                                                  std::complex<double> p0)
{
    return nicholsPoint(point.gain, point.zeros, point.poles, w, p0);
}

NicholsBox NaturalIntervalExtension::nicholsPoint(double gain, const std::vector<double> & zeros,
                                                  const std::vector<double> & poles, double w,
                                                  std::complex<double> p0)
{
    return nicholsOf(Interval(gain), factorsOf(zeros, poles, w), p0);
}

NaturalIntervalExtension::Factors NaturalIntervalExtension::factorsOf(LtiSystem * controller, double w)
{
    ensureSupportedStructure(controller->type());

    return {factorProduct(controller->numerator(), w), factorProduct(controller->denominator(), w)};
}

NaturalIntervalExtension::Factors NaturalIntervalExtension::factorsOf(const std::vector<double> & zeros,
                                                                      const std::vector<double> & poles,
                                                                      double w)
{
    return {factorProduct(zeros, w), factorProduct(poles, w)};
}

NicholsBox NaturalIntervalExtension::nicholsOf(const Interval & gain, const Factors & factors,
                                               std::complex<double> p0)
{
    const PolarInterval nominal(p0);
    const Interval modulus = loopModulus(gain, factors.numerator.magnitude(), nominal.magnitude(),
                                         factors.denominator.magnitude());
    const Interval phase = loopPhase(gain, factors.numerator.phase(), nominal.phase(), factors.denominator.phase());

    return {decibelsOf(modulus), onTheBranch(phase)};
}

NaturalIntervalExtension::BoxSquares NaturalIntervalExtension::squaresOf(LtiSystem * controller)
{
    ensureSupportedStructure(controller->type());

    BoxSquares squares;
    squares.zeros.reserve(controller->numerator().size());
    for (Parameter & parameter : controller->numerator()) {
        squares.zeros.push_back(sqr(parameterInterval(parameter)));
    }
    squares.poles.reserve(controller->denominator().size());
    for (Parameter & parameter : controller->denominator()) {
        squares.poles.push_back(sqr(parameterInterval(parameter)));
    }
    squares.gain = parameterInterval(controller->gain());
    return squares;
}

bool NaturalIntervalExtension::mayReachUnitModulus(const BoxSquares & box, const Interval & wSquared,
                                                   const Interval & nominalModulus)
{
    Interval numerator(1.0);
    for (const Interval & square : box.zeros) {
        numerator = numerator * factorModulusOfSquare(square, wSquared);
    }
    Interval denominator(1.0);
    for (const Interval & square : box.poles) {
        denominator = denominator * factorModulusOfSquare(square, wSquared);
    }

    const Interval modulus = clampedModulus(loopModulus(box.gain, numerator, nominalModulus, denominator));
    return modulus.lower() <= 1.0 && modulus.upper() >= 1.0;
}

NicholsBox NaturalIntervalExtension::numeratorTermBox(Parameter & zero, double w, std::complex<double> p0)
{
    return toNichols(factor(parameterInterval(zero), w) * PolarInterval(p0));
}

NicholsBox NaturalIntervalExtension::denominatorTermBox(Parameter & pole, double w, std::complex<double> p0)
{
    return toNichols(PolarInterval(p0) / factor(parameterInterval(pole), w));
}

NicholsBox NaturalIntervalExtension::gainTermBox(Parameter & gain, std::complex<double> p0)
{
    return toNichols(parameterInterval(gain) * PolarInterval(p0));
}

}
