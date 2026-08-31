#include "natural_interval_extension.h"

#include <cmath>

#include "Modelo/Herramientas/exception.h"

using cxsc::cinterval;
using cxsc::complex;
using cxsc::interval;
using cxsc::real;

namespace qftbx {

namespace {

const qreal kTwoPi = 2.0 * M_PI;
const qreal kRadToDeg = 180.0 / M_PI;

interval parameterInterval(Parameter * parameter)
{
    if (parameter->isUncertain()) {
        return interval(parameter->range().x(), parameter->range().y());
    }
    return interval(parameter->nominal());
}

//The projection below multiplies (jw + parameter) factors: the thesis'
//zero-pole-gain controller structure. TimeConstantGain controllers are
//accepted and projected THE SAME WAY, as historically.
// BUG: a TimeConstantGain controller evaluates as k (s/z + 1)/(s/p + 1)
// everywhere else (GUI, persistence), so for it the optimiser and the
// viewer disagree by the gain factor prod(p)/prod(z). To resolve in 8b.2:
// project time-constant factors as such, or restrict the loop-shaping
// dialog to the zero-pole-gain structure. PolynomialForm and FreeForm
// parameters are not zeros/poles at all (the historical code projected
// them as if they were, silently producing noise): they are rejected.
void ensureSupportedStructure(LtiSystem::SystemType type)
{
    if (type != LtiSystem::SystemType::ZeroPoleGain &&
        type != LtiSystem::SystemType::TimeConstantGain) {
        throw InvalidInput("Loop shaping needs a zero-pole-gain or "
                           "time-constant controller structure: the interval "
                           "projection of polynomial or free-form structures "
                           "is not implemented.");
    }
}

} // namespace

cinterval NaturalIntervalExtension::factorProduct(QVector<Parameter*> * parameters,
                                                  qreal w)
{
    //Neutral element: an empty vector stands for the constant 1 (the
    //historical code left the product UNINITIALIZED for a pure-gain
    //controller and read garbage).
    cinterval product(interval(1.0), interval(0.0));
    const complex jw(0.0, w);

    foreach (Parameter * parameter, *parameters) {
        product = product * (jw + parameterInterval(parameter));
    }

    return product;
}

interval NaturalIntervalExtension::argEnclosure(const cinterval & z)
{
    const qreal r0 = cxsc::_double(InfRe(z));
    const qreal r1 = cxsc::_double(SupRe(z));
    const qreal i0 = cxsc::_double(InfIm(z));
    const qreal i1 = cxsc::_double(SupIm(z));

    //The rectangle is closed: a corner ON an axis belongs to both
    //neighbouring cases, whose enclosures agree there.
    if (r0 >= 0 && i0 >= 0) {                          //first quadrant
        return interval(std::atan2(i0, r1) - kTwoPi, std::atan2(i1, r0) - kTwoPi);
    }
    if (r0 >= 0 && i1 <= 0) {                          //fourth quadrant
        return interval(std::atan2(i0, r0), std::atan2(i1, r1));
    }
    if (r0 >= 0) {                                     //crosses +Re axis
        //The set [atan2(i0,r0), atan2(i1,r0)] crosses the 0/-360 branch
        //cut: the only single-interval enclosure inside the branch is the
        //whole branch. (The historical mapping returned the COMPLEMENTARY
        //arc here, excluding true phases.)
        return interval(-kTwoPi, 0.0);
    }
    if (r1 <= 0 && i1 <= 0) {                          //third quadrant
        return interval(std::atan2(i1, r0), std::atan2(i0, r1));
    }
    if (r1 <= 0 && i0 >= 0) {                          //second quadrant
        return interval(std::atan2(i1, r1) - kTwoPi, std::atan2(i0, r0) - kTwoPi);
    }
    if (r1 <= 0) {                                     //crosses -Re axis
        return interval(std::atan2(i1, r1) - kTwoPi, std::atan2(i0, r1));
    }
    if (i0 >= 0) {                                     //crosses +Im axis
        return interval(std::atan2(i0, r1) - kTwoPi, std::atan2(i0, r0) - kTwoPi);
    }
    if (i1 <= 0) {                                     //crosses -Im axis
        return interval(std::atan2(i1, r0), std::atan2(i1, r1));
    }

    return interval(-kTwoPi, 0.0);                     //origin inside
}

interval NaturalIntervalExtension::wrapToBranch(const interval & theta)
{
    const qreal low = cxsc::_double(Inf(theta));
    const qreal high = cxsc::_double(Sup(theta));

    if (high - low >= kTwoPi) {
        return interval(-kTwoPi, 0.0);
    }
    if (low >= 0.0) {
        //Entirely positive: one branch down.
        return interval(low - kTwoPi, high - kTwoPi);
    }
    if (high > 0.0 || low < -kTwoPi) {
        //Crosses the branch cut: the set splits inside (-2*pi, 0] and only
        //the whole branch encloses it (see argEnclosure).
        return interval(-kTwoPi, 0.0);
    }
    return theta;
}

interval NaturalIntervalExtension::toDecibel(interval magnitude)
{
    //log10 must never see 0 or infinity: fi_lib's q_log ABORTS the process
    //on them (observed live once an interval product overflowed). Clamping
    //to (0, MaxReal] keeps every representable true value enclosed.
    if (Sup(magnitude) > cxsc::MaxReal) {
        SetSup(magnitude, cxsc::MaxReal);
    }
    if (Sup(magnitude) <= 0.0) {
        SetSup(magnitude, cxsc::MinReal);
    }
    if (Inf(magnitude) <= 0.0) {
        SetInf(magnitude, cxsc::MinReal);
    }

    return 20.0 * log10(magnitude);
}

cinterval NaturalIntervalExtension::nicholsBox(LtiSystem * controller, qreal w,
                                               complex p0, bool nyquist)
{
    ensureSupportedStructure(controller->type());

    const cinterval numerator = factorProduct(controller->numerator(), w);
    const cinterval denominator = factorProduct(controller->denominator(), w);

    const cinterval a = parameterInterval(controller->gain()) * numerator * p0;

    //Magnitude and phase by interval arithmetic (thesis section 1.2.5):
    //|a| / |den| and arg(a) - arg(den), the phase mapped back onto the
    //(-360, 0] branch. The historical remapping through a complex-plane
    //rectangle inflated the box AND mangled sets crossing the branch cut.
    const interval magnitude = abs(a) / abs(denominator);
    const interval theta = wrapToBranch(argEnclosure(a) - argEnclosure(denominator));

    if (nyquist) {
        m_nyquistPhaseInf = cxsc::_double(Inf(theta));
        m_nyquistDecibelBox = cinterval(toDecibel(magnitude), theta * kRadToDeg);

        return cinterval(magnitude * cos(theta), magnitude * sin(theta));
    }

    return cinterval(toDecibel(magnitude), theta * kRadToDeg);
}

cinterval NaturalIntervalExtension::numeratorBox(QVector<Parameter*> * numerator,
                                                 qreal w, LtiSystem::SystemType type,
                                                 bool nyquist)
{
    ensureSupportedStructure(type);

    const cinterval a = factorProduct(numerator, w);

    if (nyquist) {
        return a;
    }

    return cinterval(toDecibel(abs(a)), argEnclosure(a) * kRadToDeg);
}

cinterval NaturalIntervalExtension::denominatorBox(QVector<Parameter*> * denominator,
                                                   qreal w, LtiSystem::SystemType type,
                                                   bool nyquist)
{
    //The denominator product is enclosed as such (not inverted): the
    //caller combines it, matching the historical contract.
    return numeratorBox(denominator, w, type, nyquist);
}

cinterval NaturalIntervalExtension::numeratorTermBox(Parameter * zero, qreal w,
                                                     complex p0)
{
    const cinterval a = (complex(0.0, w) + parameterInterval(zero)) * p0;

    return cinterval(toDecibel(abs(a)), argEnclosure(a) * kRadToDeg);
}

cinterval NaturalIntervalExtension::denominatorTermBox(Parameter * pole, qreal w,
                                                       complex p0)
{
    const cinterval a = 1.0 / (complex(0.0, w) + parameterInterval(pole)) * p0;

    return cinterval(toDecibel(abs(a)), argEnclosure(a) * kRadToDeg);
}

cinterval NaturalIntervalExtension::gainTermBox(Parameter * gain, complex p0)
{
    const cinterval a = parameterInterval(gain) * p0;

    return cinterval(toDecibel(abs(a)), argEnclosure(a) * kRadToDeg);
}

qreal NaturalIntervalExtension::nyquistPhaseInf()
{
    return m_nyquistPhaseInf;
}

cinterval NaturalIntervalExtension::nyquistDecibelBox()
{
    return m_nyquistDecibelBox;
}

} // namespace qftbx
