#include "src/core/loopshaping/natural_interval_extension.h"
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

//The factor (jw + x) as a set: a horizontal segment of the complex plane,
//read in polar coordinates.
PolarInterval factor(const Interval & x, double w)
{
    return PolarInterval(ComplexInterval(x, Interval(w)));
}

//The projection below multiplies (jw + parameter) factors: the thesis'
//zero-pole-gain controller structure, and only that one. A time-constant
//controller evaluates as k (s/z + 1)/(s/p + 1) everywhere else (evaluation,
//interface, file), so projecting it as zero-pole-gain, as the historical
//code did, made the optimiser and the viewer disagree by the factor
//prod(p)/prod(z); polynomial and free-form parameters are not zeros or poles
//at all. Until the projection of those structures exists, they are refused
//with a message for the user (decision 2026-09-04).
void ensureSupportedStructure(LtiSystem::SystemType type)
{
    if (type != LtiSystem::SystemType::ZeroPoleGain) {
        throw InvalidInput("Loop shaping supports zero-pole-gain controller "
                           "structures only, for now: a time-constant, "
                           "polynomial or free-form controller structure is "
                           "not supported yet.");
    }
}

} // namespace

PolarInterval NaturalIntervalExtension::factorProduct(std::vector<Parameter> & parameters, double w)
{
    //Neutral element: an empty vector stands for the constant 1 (the
    //historical code left the product UNINITIALIZED for a pure-gain
    //controller and read garbage).
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
    //log10 must never see 0 or infinity. Clamping to the positive finite
    //doubles keeps every representable true value enclosed.
    double low = loop.magnitude().lower();
    double high = loop.magnitude().upper();

    if (high > std::numeric_limits<double>::max()) {
        high = std::numeric_limits<double>::max();
    }
    if (high <= 0.0) {
        high = std::numeric_limits<double>::min();
    }
    if (low <= 0.0) {
        low = std::numeric_limits<double>::min();
    }

    const Interval magnitudeDb = Interval(20.0) * log10(Interval(low, high));

    //The phase onto the (-2 pi, 0] branch: the set is shifted by whole
    //turns until its upper end lands in the branch; if its lower end then
    //falls below the branch, the set crosses the cut and only the whole
    //branch encloses it (the branch mapping this replaces returned the
    //COMPLEMENTARY arc there, i.e. boxes that EXCLUDED true phases).
    const Interval twoPi = Interval(2.0) * Interval::pi();
    Interval theta = loop.phase();

    if (theta.width() >= twoPi.lower()) {
        theta = Interval(-twoPi.upper(), 0.0);
    } else {
        const double turns = std::ceil(theta.upper() / twoPi.lower());
        theta = theta - Interval(turns) * twoPi;

        //Rounding may leave the upper end a hair above zero or the lower
        //end a hair below the cut when the true set touches them; the
        //whole branch is the safe answer in either case.
        if (theta.upper() > 0.0 || theta.lower() < -twoPi.upper()) {
            theta = Interval(-twoPi.upper(), 0.0);
        }
    }

    return {magnitudeDb, theta * Interval(kRadToDeg)};
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
    //Magnitudes multiply and phases add (thesis section 1.2.5); the
    //denominator's magnitude reaches zero only when a pole interval crosses
    //-jw, which a design frequency w > 0 with real poles never does.
    const PolarInterval loop = gain * (factors.numerator * PolarInterval(p0)) / factors.denominator;

    return toNichols(loop);
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

} // namespace qftbx
