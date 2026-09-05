// The interval arithmetic the loop shaping rests on: every operation must
// enclose the true result, whatever the rounding, and the polar form must
// enclose the product set of the rectangles it is built from. These are the
// acceptance tests of the arithmetic layer, whatever library sits under it.
#include <gtest/gtest.h>

#include <cfenv>
#include <cmath>
#include <complex>
#include <random>
#include <stdexcept>
#include <vector>

#include "src/core/math/constants.h"
#include "src/core/math/interval.h"

using namespace qftbx;

namespace {

bool encloses(const Interval & x, double value)
{
    return x.lower() <= value && value <= x.upper();
}

} // namespace

TEST(IntervalArithmetic, TheOperationsRoundOutwards)
{
    //The exact results are not doubles, so a rigorous enclosure has the
    //nearest double inside it and cannot be a point. The exact values are
    //checked in extended precision.
    const Interval tenth(0.1);
    const Interval sum = tenth + tenth + tenth;
    EXPECT_TRUE(encloses(sum, 0.1 + 0.1 + 0.1));
    EXPECT_GT(sum.width(), 0.0) << "three roundings cannot come out exact";
    const long double exactSum = 0.1L + 0.1L + 0.1L;
    EXPECT_LE(static_cast<long double>(sum.lower()), exactSum);
    EXPECT_GE(static_cast<long double>(sum.upper()), exactSum);

    const Interval third = Interval(1.0) / Interval(3.0);
    EXPECT_TRUE(encloses(third, 1.0 / 3.0));
    EXPECT_LT(third.lower(), third.upper());
    EXPECT_LE(static_cast<long double>(third.lower()), 1.0L / 3.0L);
    EXPECT_GE(static_cast<long double>(third.upper()), 1.0L / 3.0L);

    //The square of the double nearest 1.1, not of 1.1 itself: the point
    //interval holds the double.
    const Interval product = Interval(1.1) * Interval(1.1);
    const long double exactProduct = static_cast<long double>(1.1) * static_cast<long double>(1.1);
    EXPECT_TRUE(encloses(product, 1.1 * 1.1));
    EXPECT_LE(static_cast<long double>(product.lower()), exactProduct);
    EXPECT_GE(static_cast<long double>(product.upper()), exactProduct);
    EXPECT_GT(product.width(), 0.0);
}

TEST(IntervalArithmetic, TheRoundingModeIsNeverTouched)
{
    //The arithmetic emulates the directed roundings: the floating-point
    //environment is the same before and after, which is what lets it run
    //inside OpenMP regions and next to Qt without a compiler flag.
    std::fesetround(FE_TONEAREST);
    Interval x(1.0, 2.0);
    for (int i = 0; i < 100; ++i) {
        x = sqrt(x * Interval(3.0) + Interval(0.1)) / Interval(1.7);
        x = log(x + Interval(1.0)) + atan(x);
    }
    EXPECT_EQ(std::fegetround(), FE_TONEAREST);
    EXPECT_DOUBLE_EQ(0.1 + 0.2, 0.30000000000000004) << "plain arithmetic still rounds to nearest";
}

TEST(IntervalArithmetic, TheFunctionsEncloseTheirValues)
{
    EXPECT_TRUE(encloses(sqrt(Interval(2.0)), std::sqrt(2.0)));
    EXPECT_TRUE(encloses(sqr(Interval(-3.0, 2.0)), 0.0));
    EXPECT_DOUBLE_EQ(sqr(Interval(-3.0, 2.0)).upper(), 9.0);
    EXPECT_DOUBLE_EQ(sqr(Interval(-3.0, 2.0)).lower(), 0.0);
    EXPECT_TRUE(encloses(log10(Interval(1000.0)), 3.0));
    EXPECT_TRUE(encloses(log2(Interval(8.0)), 3.0));
    EXPECT_TRUE(encloses(exp(Interval(1.0)), qftbx::math::kE));
    EXPECT_TRUE(std::isinf(exp(Interval(0.0, 1000.0)).upper()));
    EXPECT_TRUE(encloses(atan(Interval(1.0)), qftbx::math::kPi / 4.0));
    EXPECT_TRUE(encloses(Interval::pi(), qftbx::math::kPi));
    EXPECT_LT(Interval::pi().width(), 1e-15);
    EXPECT_TRUE(encloses(abs(Interval(-2.0, -1.0)), 1.5));
    EXPECT_TRUE(encloses(pow(Interval(2.0), 10), 1024.0));
    EXPECT_TRUE(encloses(pow(Interval(2.0), Interval(0.5)), std::sqrt(2.0)));
    EXPECT_TRUE(encloses(cos(Interval(0.0, 4.0)), -1.0)) << "the minimum inside the range is reached";
    EXPECT_TRUE(encloses(sin(Interval(0.0, 4.0)), 1.0));
    EXPECT_TRUE(encloses(tanh(Interval(1.0)), std::tanh(1.0)));
}

TEST(IntervalArithmetic, TheLibraryFunctionsAreWidenedEnough)
{
    //The logarithms and arc tangents are the C library's doubles widened by
    //a few ulps; the extended-precision values must fall inside, at many
    //arguments and across every scale the projection meets (magnitudes
    //from 1e-6 to 1e6, phases from any quadrant).
    std::mt19937 generator(3);
    std::uniform_real_distribution<double> exponent(-6.0, 6.0);
    std::uniform_real_distribution<double> coordinate(-10.0, 10.0);

    for (int i = 0; i < 20000; ++i) {
        const double x = std::pow(10.0, exponent(generator));
        const Interval point(x);
        EXPECT_LE(static_cast<long double>(exp(Interval(-x)).lower()), std::exp(static_cast<long double>(-x)));
        EXPECT_GE(static_cast<long double>(exp(Interval(-x)).upper()), std::exp(static_cast<long double>(-x)));
        EXPECT_LE(static_cast<long double>(log(point).lower()), std::log(static_cast<long double>(x)));
        EXPECT_GE(static_cast<long double>(log(point).upper()), std::log(static_cast<long double>(x)));
        EXPECT_LE(static_cast<long double>(log10(point).lower()), std::log10(static_cast<long double>(x)));
        EXPECT_GE(static_cast<long double>(log10(point).upper()), std::log10(static_cast<long double>(x)));
        EXPECT_LE(static_cast<long double>(log2(point).lower()), std::log2(static_cast<long double>(x)));
        EXPECT_GE(static_cast<long double>(log2(point).upper()), std::log2(static_cast<long double>(x)));

        const double re = coordinate(generator);
        const double im = coordinate(generator);
        const Interval angle = atan2(Interval(im), Interval(re));
        const long double exact = std::atan2(static_cast<long double>(im), static_cast<long double>(re));
        EXPECT_LE(static_cast<long double>(angle.lower()), exact);
        EXPECT_GE(static_cast<long double>(angle.upper()), exact);
        EXPECT_LE(static_cast<long double>(atan(Interval(re)).lower()), std::atan(static_cast<long double>(re)));
        EXPECT_GE(static_cast<long double>(atan(Interval(re)).upper()), std::atan(static_cast<long double>(re)));
    }

    //Exact arguments give exact-looking answers, still widened.
    EXPECT_LT(log(Interval(1.0)).lower(), 0.0);
    EXPECT_GT(log(Interval(1.0)).upper(), 0.0);
    EXPECT_LT(log(Interval(1.0)).width(), 1e-300);
}

TEST(IntervalArithmetic, TheWideningStepsFourUlpsEitherWay)
{
    //The widening moves a value four representable doubles: the same as
    //four nextafter steps, on either side of zero, at every scale, and at
    //the edges where the integer step is not taken.
    std::mt19937 generator(9);
    std::uniform_real_distribution<double> exponent(-300.0, 300.0);
    std::uniform_int_distribution<int> sign(0, 1);
    const auto stepped = [](double value, int steps, double direction) {
        for (int i = 0; i < steps; ++i) {
            value = std::nextafter(value, direction);
        }
        return value;
    };
    const double inf = std::numeric_limits<double>::infinity();

    for (int i = 0; i < 5000; ++i) {
        const double value = (sign(generator) ? -1.0 : 1.0) * std::pow(10.0, exponent(generator));
        EXPECT_EQ(detail::downwards(value), stepped(value, detail::kLibraryUlps, -inf)) << value;
        EXPECT_EQ(detail::upwards(value), stepped(value, detail::kLibraryUlps, inf)) << value;
    }
    for (const double value : {0.0, -0.0, std::numeric_limits<double>::denorm_min(), -std::numeric_limits<double>::denorm_min(),
                               std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), inf, -inf}) {
        EXPECT_EQ(detail::downwards(value), stepped(value, detail::kLibraryUlps, -inf)) << value;
        EXPECT_EQ(detail::upwards(value), stepped(value, detail::kLibraryUlps, inf)) << value;
    }
}

TEST(IntervalArithmetic, TheTrigonometricFunctionsEncloseTheirRangesOverIntervals)
{
    //Random intervals up to a few turns wide: every sampled value of sin,
    //cos and tan lies inside, and the extremes are found wherever a
    //maximum or minimum falls inside.
    std::mt19937 generator(5);
    std::uniform_real_distribution<double> start(-20.0, 20.0);
    std::uniform_real_distribution<double> width(0.0, 8.0);
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    for (int i = 0; i < 3000; ++i) {
        const double a = start(generator);
        const double b = a + width(generator);
        const Interval x(a, b);
        const Interval s = sin(x);
        const Interval c = cos(x);
        const Interval t = tan(x);

        for (int j = 0; j < 20; ++j) {
            const double point = a + (b - a) * unit(generator);
            const long double exact = static_cast<long double>(point);
            EXPECT_TRUE(encloses(s, static_cast<double>(std::sin(exact)))) << "sin " << x << " at " << point;
            EXPECT_TRUE(encloses(c, static_cast<double>(std::cos(exact)))) << "cos " << x << " at " << point;
            EXPECT_TRUE(encloses(t, static_cast<double>(std::tan(exact)))) << "tan " << x << " at " << point;
        }

        EXPECT_GE(s.lower(), -1.0);
        EXPECT_LE(s.upper(), 1.0);
        if (b - a >= 2.0 * qftbx::math::kPi) {
            EXPECT_DOUBLE_EQ(s.lower(), -1.0);
            EXPECT_DOUBLE_EQ(c.upper(), 1.0);
        }
    }

    //A pole inside tan gives the whole line; a maximum at pi/2 inside sin
    //gives exactly 1 above.
    EXPECT_TRUE(std::isinf(tan(Interval(1.0, 2.0)).upper()));
    EXPECT_DOUBLE_EQ(sin(Interval(1.0, 2.0)).upper(), 1.0);
    EXPECT_LT(sin(Interval(1.0, 2.0)).lower(), std::sin(1.0));
    EXPECT_GT(tan(Interval(0.5, 1.0)).lower(), 0.5);

    //The inverses are monotone on [-1, 1] and refuse anything outside.
    EXPECT_TRUE(encloses(asin(Interval(0.5)), std::asin(0.5)));
    EXPECT_TRUE(encloses(acos(Interval(-0.5, 0.5)), std::acos(0.25)));
    EXPECT_TRUE(encloses(asin(Interval(1.0)), qftbx::math::kPi / 2.0));
    EXPECT_TRUE(encloses(acos(Interval(-1.0)), qftbx::math::kPi));
    EXPECT_THROW(asin(Interval(0.0, 1.5)), std::domain_error);
    EXPECT_THROW(acos(Interval(-2.0, 0.0)), std::domain_error);
}

TEST(IntervalArithmetic, DomainErrorsAreReportedNotAborted)
{
    EXPECT_THROW(Interval(1.0) / Interval(-1.0, 1.0), std::domain_error);
    EXPECT_THROW(sqrt(Interval(-2.0, -1.0)), std::domain_error);
    EXPECT_THROW(log(Interval(-2.0, -1.0)), std::domain_error);
}

TEST(IntervalArithmetic, IntersectionAndHull)
{
    const Interval a(0.0, 2.0);
    const Interval b(1.0, 3.0);
    const std::optional<Interval> common = a.intersection(b);
    ASSERT_TRUE(common.has_value());
    EXPECT_DOUBLE_EQ(common->lower(), 1.0);
    EXPECT_DOUBLE_EQ(common->upper(), 2.0);
    EXPECT_FALSE(a.intersection(Interval(5.0, 6.0)).has_value());

    const Interval both = Interval::hull(a, Interval(5.0, 6.0));
    EXPECT_DOUBLE_EQ(both.lower(), 0.0);
    EXPECT_DOUBLE_EQ(both.upper(), 6.0);

    EXPECT_TRUE(Interval(2.0, 1.0).contains(1.5)) << "the bounds may come in either order";
}

TEST(ComplexIntervalArithmetic, MagnitudeAndPhaseOfARectangleEncloseItsPoints)
{
    const ComplexInterval z(Interval(1.0, 2.0), Interval(-1.0, 3.0));
    const Interval magnitude = z.magnitude();
    const Interval phase = z.phase();

    std::mt19937 generator(7);
    std::uniform_real_distribution<double> re(1.0, 2.0);
    std::uniform_real_distribution<double> im(-1.0, 3.0);
    for (int i = 0; i < 2000; ++i) {
        const std::complex<double> point(re(generator), im(generator));
        EXPECT_TRUE(encloses(magnitude, std::abs(point)));
        EXPECT_TRUE(encloses(phase, std::arg(point)));
    }

    //The corners and the nearest point are the extremes.
    EXPECT_DOUBLE_EQ(magnitude.lower(), 1.0) << "the nearest point is (1, 0)";
    EXPECT_NEAR(magnitude.upper(), std::hypot(2.0, 3.0), 1e-15);
}

TEST(ComplexIntervalArithmetic, ARectangleAcrossTheNegativeAxisHasAContinuousPhase)
{
    //Straddling the negative real axis, the phase runs through pi without
    //splitting into two pieces at +pi/-pi.
    const ComplexInterval z(Interval(-2.0, -1.0), Interval(-0.5, 0.5));
    const Interval phase = z.phase();
    EXPECT_LT(phase.width(), 1.0);
    EXPECT_TRUE(encloses(phase, qftbx::math::kPi) || encloses(phase, -qftbx::math::kPi));

    const ComplexInterval origin(Interval(-1.0, 1.0), Interval(-1.0, 1.0));
    EXPECT_TRUE(origin.containsOrigin());
    EXPECT_GE(origin.phase().width(), 2.0 * qftbx::math::kPi - 1e-15);
    EXPECT_DOUBLE_EQ(origin.magnitude().lower(), 0.0);
}

TEST(PolarIntervalArithmetic, TheProductEnclosesTheProductSet)
{
    //Two factors of a loop transmission, (jw + z) with z in a box and a
    //fixed complex plant value: every product of a sampled zero and the
    //plant lies inside the polar product.
    const double w = 2.0;
    const Interval zero(0.5, 4.0);
    const Interval pole(1.0, 10.0);
    const std::complex<double> plant(-0.3, -1.7);

    const PolarInterval factor(ComplexInterval(zero, Interval(w)));
    const PolarInterval divisor(ComplexInterval(pole, Interval(w)));
    const PolarInterval loop = PolarInterval(plant) * factor / divisor;

    std::mt19937 generator(11);
    std::uniform_real_distribution<double> zeros(0.5, 4.0);
    std::uniform_real_distribution<double> poles(1.0, 10.0);
    for (int i = 0; i < 2000; ++i) {
        const std::complex<double> value = plant * std::complex<double>(zeros(generator), w)
                                           / std::complex<double>(poles(generator), w);
        EXPECT_TRUE(encloses(loop.magnitude(), std::abs(value)));

        //The phase of the product set, compared modulo a turn.
        const double phase = std::arg(value);
        bool inside = false;
        for (int turn = -2; turn <= 2 && !inside; ++turn) {
            inside = encloses(loop.phase(), phase + 2.0 * qftbx::math::kPi * turn);
        }
        EXPECT_TRUE(inside);
    }
}

TEST(PolarIntervalArithmetic, ARealScaleKeepsOrTurnsThePhase)
{
    const PolarInterval z(Interval(1.0, 2.0), Interval(0.1, 0.2));

    const PolarInterval scaled = Interval(3.0, 4.0) * z;
    EXPECT_DOUBLE_EQ(scaled.magnitude().lower(), 3.0);
    EXPECT_DOUBLE_EQ(scaled.magnitude().upper(), 8.0);
    EXPECT_EQ(scaled.phase(), z.phase());

    const PolarInterval turned = Interval(-1.0) * z;
    EXPECT_TRUE(encloses(turned.phase(), 0.15 + qftbx::math::kPi));

    EXPECT_THROW(PolarInterval(Interval(-1.0, 1.0), Interval(0.0)), std::domain_error);
    EXPECT_THROW(z / PolarInterval(Interval(0.0, 1.0), Interval(0.0)), std::domain_error);
}
