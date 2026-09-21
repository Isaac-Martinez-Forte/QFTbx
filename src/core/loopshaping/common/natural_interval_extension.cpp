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
 * answer whenever the set crosses the cut or rounding puts an end past it.
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

PolarInterval factor(const Interval & x, double w)
{
    return PolarInterval(ComplexInterval(x, Interval(w)));
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

    const Interval twoPi = Interval(2.0) * Interval::pi();
    Interval theta = loop.phase();

    if (theta.width() >= twoPi.lower()) {
        theta = Interval(-twoPi.upper(), 0.0);
    } else {
        const double turns = std::ceil(theta.upper() / twoPi.lower());
        theta = theta - Interval(turns) * twoPi;

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

}
