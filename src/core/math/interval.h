/**
 * @file
 * @brief The interval arithmetic of the toolbox.
 *
 * Three types, the only ones the rest of the code sees: Interval, a closed
 * real interval with outward rounding and rigorous elementary functions;
 * ComplexInterval, a rectangle of the complex plane, whose magnitude() and
 * phase() are rigorous readings of it; and PolarInterval, a magnitude and a
 * phase interval, in which products and quotients keep their shape, so the
 * loop k P0 prod(jw + z) / prod(jw + p) is assembled in polar form.
 *
 * The arithmetic underneath is kv (3rd-party/kv), by default, in its
 * rounding-emulation mode, or C-XSC, chosen at configuration time
 * (QFTBX_INTERVAL_BACKEND) and confined to detail::Backend, which supplies
 * the four operations, the square root, the integer power and pi.
 * Everything else is written here over those, so the two give the same
 * enclosures up to rounding.
 *
 * The exponential, the logarithms and the trigonometric functions and their
 * inverses take the C library's values widened by four ulps on each side
 * (detail::kLibraryUlps), twice the largest error glibc lists for them. The
 * monotone ones take the values at the ends; sin and cos add the extremes
 * +1 and -1 wherever a maximum or a minimum lies inside, located with the
 * enclosure of pi; tan is the whole line where the interval may hold a
 * pole. sqr is tight, [0, max] where x straddles zero. atan2 is the
 * argument of the rectangle: the whole turn when it contains the origin,
 * otherwise taken at its corners, and continuous past pi when it crosses
 * the negative real axis. pi() and e() are enclosures of the constants.
 *
 * A domain error throws std::domain_error in both backends. The phase of a
 * ComplexInterval is continuous and may reach past pi; that of a
 * PolarInterval is not reduced modulo a turn, so a long product can span
 * more than 2 pi and the caller maps it onto its branch. A negative scale
 * turns the phase by pi, and a scale straddling zero takes the hull of
 * both.
 */

#ifndef QFTBX_MATH_INTERVAL_H
#define QFTBX_MATH_INTERVAL_H

#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>

#if defined(QFTBX_INTERVAL_CXSC)
#include <interval.hpp>
#include <imath.hpp>
#else
#include <kv/interval.hpp>
#include <kv/rdouble.hpp>
#endif

namespace qftbx {

namespace detail {

#if defined(QFTBX_INTERVAL_CXSC)

using Backend = cxsc::interval;

inline Backend backend(double lower, double upper) { return cxsc::interval(lower, upper); }
inline double lowerOf(const Backend & x) { return cxsc::_double(cxsc::Inf(x)); }
inline double upperOf(const Backend & x) { return cxsc::_double(cxsc::Sup(x)); }
inline double widthOf(const Backend & x) { return cxsc::_double(cxsc::diam(x)); }
inline Backend sqrtOf(const Backend & x) { return cxsc::sqrt(x); }
inline Backend powerOf(const Backend & x, int n) { return cxsc::power(x, n); }
inline Backend piOf() { return cxsc::Pi(); }
inline Backend squaresOf(double nearer, double farther)
{
    return cxsc::interval(cxsc::Inf(backend(nearer, nearer) * backend(nearer, nearer)),
                          cxsc::Sup(backend(farther, farther) * backend(farther, farther)));
}

#else

using Backend = kv::interval<double>;

inline Backend backend(double lower, double upper) { return kv::interval<double>(lower, upper); }
inline double lowerOf(const Backend & x) { return x.lower(); }
inline double upperOf(const Backend & x) { return x.upper(); }
inline double widthOf(const Backend & x) { return width(x); }
inline Backend sqrtOf(const Backend & x) { return sqrt(x); }
inline Backend powerOf(const Backend & x, int n) { return pow(x, n); }
inline Backend piOf() { return kv::constants<kv::interval<double>>::pi(); }
inline Backend squaresOf(double nearer, double farther)
{
    return kv::interval<double>(kv::rop<double>::mul_down(nearer, nearer), kv::rop<double>::mul_up(farther, farther));
}

#endif

constexpr int kLibraryUlps = 4;

constexpr double kLargestReducedArgument = 1e15;

inline double awayFromZero(double value)
{
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    bits += kLibraryUlps;
    double moved;
    std::memcpy(&moved, &bits, sizeof moved);
    return moved;
}

inline double towardsZero(double value)
{
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    bits -= kLibraryUlps;
    double moved;
    std::memcpy(&moved, &bits, sizeof moved);
    return moved;
}

inline bool stepsSafely(double value)
{
    return std::isfinite(value) && std::fabs(value) > kLibraryUlps * std::numeric_limits<double>::denorm_min()
           && std::fabs(value) < std::numeric_limits<double>::max();
}

inline double downwards(double value)
{
    if (stepsSafely(value)) {
        return value > 0.0 ? towardsZero(value) : awayFromZero(value);
    }
    for (int i = 0; i < kLibraryUlps; ++i) {
        value = std::nextafter(value, -std::numeric_limits<double>::infinity());
    }
    return value;
}

inline double upwards(double value)
{
    if (stepsSafely(value)) {
        return value > 0.0 ? awayFromZero(value) : towardsZero(value);
    }
    for (int i = 0; i < kLibraryUlps; ++i) {
        value = std::nextafter(value, std::numeric_limits<double>::infinity());
    }
    return value;
}

}

class Interval
{
public:
    Interval() : m_value(detail::backend(0.0, 0.0)) {}

    Interval(double value) : m_value(detail::backend(value, value)) {}

    Interval(double lower, double upper)
        : m_value(detail::backend(lower <= upper ? lower : upper, lower <= upper ? upper : lower)) {}

    double lower() const { return detail::lowerOf(m_value); }
    double upper() const { return detail::upperOf(m_value); }

    double width() const { return detail::widthOf(m_value); }

    double midpoint() const { return lower() + (upper() - lower()) / 2.0; }

    bool isPoint() const { return lower() == upper(); }

    bool contains(double value) const { return lower() <= value && value <= upper(); }
    bool contains(const Interval & other) const { return lower() <= other.lower() && other.upper() <= upper(); }
    bool containsZero() const { return contains(0.0); }
    bool intersects(const Interval & other) const { return lower() <= other.upper() && other.lower() <= upper(); }

    std::optional<Interval> intersection(const Interval & other) const
    {
        if (!intersects(other)) {
            return std::nullopt;
        }
        return Interval(std::fmax(lower(), other.lower()), std::fmin(upper(), other.upper()));
    }

    static Interval hull(const Interval & a, const Interval & b)
    {
        return Interval(std::fmin(a.lower(), b.lower()), std::fmax(a.upper(), b.upper()));
    }

    static Interval pi() { return Interval(detail::piOf()); }
    static Interval e() { return exp(Interval(1.0)); }

    friend Interval operator+(const Interval & a, const Interval & b) { return Interval(a.m_value + b.m_value); }
    friend Interval operator-(const Interval & a, const Interval & b) { return Interval(a.m_value - b.m_value); }
    friend Interval operator*(const Interval & a, const Interval & b) { return Interval(a.m_value * b.m_value); }
    friend Interval operator-(const Interval & a) { return Interval(-a.m_value); }

    friend Interval operator/(const Interval & a, const Interval & b)
    {
        if (b.containsZero()) {
            throw std::domain_error("Interval: division by an interval containing zero");
        }
        return Interval(a.m_value / b.m_value);
    }

    Interval & operator+=(const Interval & b) { return *this = *this + b; }
    Interval & operator-=(const Interval & b) { return *this = *this - b; }
    Interval & operator*=(const Interval & b) { return *this = *this * b; }
    Interval & operator/=(const Interval & b) { return *this = *this / b; }

    friend bool operator==(const Interval & a, const Interval & b) { return a.lower() == b.lower() && a.upper() == b.upper(); }
    friend bool operator!=(const Interval & a, const Interval & b) { return !(a == b); }

    friend Interval sqrt(const Interval & x)
    {
        if (x.lower() < 0.0) {
            throw std::domain_error("sqrt: the interval reaches below zero");
        }
        return Interval(detail::sqrtOf(x.m_value));
    }

    friend Interval exp(const Interval & x)
    {
        return Interval(detail::downwards(std::exp(x.lower())), detail::upwards(std::exp(x.upper())));
    }

    friend Interval log(const Interval & x) { return positiveMonotone(x, std::log, "log"); }
    friend Interval log10(const Interval & x) { return positiveMonotone(x, std::log10, "log10"); }
    friend Interval log2(const Interval & x) { return positiveMonotone(x, std::log2, "log2"); }
    friend Interval atan(const Interval & x)
    {
        return Interval(detail::downwards(std::atan(x.lower())), detail::upwards(std::atan(x.upper())));
    }

    friend Interval sin(const Interval & x) { return periodic(x, std::sin, 0.5); }
    friend Interval cos(const Interval & x) { return periodic(x, std::cos, 0.0); }

    friend Interval tan(const Interval & x)
    {
        if (!std::isfinite(x.lower()) || !std::isfinite(x.upper())
                || std::fabs(x.lower()) > detail::kLargestReducedArgument
                || std::fabs(x.upper()) > detail::kLargestReducedArgument
                || x.width() >= pi().upper() || containsAMultipleOfPi(x, 0.5).has_value()) {
            return whole();
        }
        return Interval(detail::downwards(std::tan(x.lower())), detail::upwards(std::tan(x.upper())));
    }

    friend Interval asin(const Interval & x)
    {
        ensureUnitDomain(x, "asin");
        return Interval(detail::downwards(std::asin(x.lower())), detail::upwards(std::asin(x.upper())));
    }
    friend Interval acos(const Interval & x)
    {
        ensureUnitDomain(x, "acos");
        return Interval(detail::downwards(std::acos(x.upper())), detail::upwards(std::acos(x.lower())));
    }
    friend Interval sinh(const Interval & x) { return Interval(sinh(x.m_value)); }
    friend Interval cosh(const Interval & x) { return Interval(cosh(x.m_value)); }
    friend Interval tanh(const Interval & x) { return Interval(tanh(x.m_value)); }
    friend Interval abs(const Interval & x)
    {
        if (x.lower() >= 0.0) {
            return x;
        }
        if (x.upper() <= 0.0) {
            return -x;
        }
        return Interval(0.0, std::fmax(-x.lower(), x.upper()));
    }

    friend Interval pow(const Interval & x, int n)
    {
        if (n < 0 && x.containsZero()) {
            throw std::domain_error("pow: a negative power of an interval containing zero");
        }
        return Interval(detail::powerOf(x.m_value, n));
    }

    friend Interval pow(const Interval & x, const Interval & y) { return exp(y * log(x)); }

    friend Interval atan2(const Interval & y, const Interval & x)
    {
        if (x.containsZero() && y.containsZero()) {
            return Interval(-pi().upper(), pi().upper());
        }

        const bool acrossTheNegativeAxis = x.upper() < 0.0 && y.containsZero();
        double lowest = std::numeric_limits<double>::infinity();
        double highest = -std::numeric_limits<double>::infinity();

        const double xs[2] = {x.lower(), x.upper()};
        const double ys[2] = {y.lower(), y.upper()};
        const int xEnds = std::memcmp(&xs[0], &xs[1], sizeof(double)) == 0 ? 1 : 2;
        const int yEnds = std::memcmp(&ys[0], &ys[1], sizeof(double)) == 0 ? 1 : 2;
        for (int i = 0; i < xEnds; ++i) {
            for (int j = 0; j < yEnds; ++j) {
                const double angle = acrossTheNegativeAxis ? std::atan2(-ys[j], -xs[i]) : std::atan2(ys[j], xs[i]);
                lowest = std::fmin(lowest, angle);
                highest = std::fmax(highest, angle);
            }
        }

        const Interval corners(detail::downwards(lowest), detail::upwards(highest));
        return acrossTheNegativeAxis ? pi() + corners : corners;
    }

    friend Interval sqr(const Interval & x)
    {
        const double lo = x.lower();
        const double hi = x.upper();

        if (lo >= 0.0) {
            return Interval(detail::squaresOf(lo, hi));
        }
        if (hi <= 0.0) {
            return Interval(detail::squaresOf(hi, lo));
        }

        const Interval a = Interval(lo) * Interval(lo);
        const Interval b = Interval(hi) * Interval(hi);
        return Interval(0.0, std::fmax(a.upper(), b.upper()));
    }

    friend std::ostream & operator<<(std::ostream & out, const Interval & x)
    {
        return out << "[" << x.lower() << ", " << x.upper() << "]";
    }

private:
    explicit Interval(const detail::Backend & value) : m_value(value) {}

    static Interval whole()
    {
        return Interval(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    }

    static Interval positiveMonotone(const Interval & x, double (*function)(double), const char * name)
    {
        if (x.lower() <= 0.0) {
            throw std::domain_error(std::string(name) + ": the interval is not positive");
        }
        return Interval(detail::downwards(function(x.lower())), detail::upwards(function(x.upper())));
    }

    static void ensureUnitDomain(const Interval & x, const char * name)
    {
        if (x.lower() < -1.0 || x.upper() > 1.0) {
            throw std::domain_error(std::string(name) + ": the interval leaves [-1, 1]");
        }
    }

    static std::optional<double> containsAMultipleOfPi(const Interval & x, double offset)
    {
        const Interval piValue = pi();
        const double first = std::floor(x.lower() / piValue.lower() - offset) - 1.0;
        const double last = std::ceil(x.upper() / piValue.lower() - offset) + 1.0;

        for (double k = first; k <= last; k += 1.0) {
            if ((Interval(k + offset) * piValue).intersects(x)) {
                return k;
            }
        }
        return std::nullopt;
    }

    static Interval periodic(const Interval & x, double (*function)(double), double offset)
    {
        if (!std::isfinite(x.lower()) || !std::isfinite(x.upper())
                || std::fabs(x.lower()) > detail::kLargestReducedArgument
                || std::fabs(x.upper()) > detail::kLargestReducedArgument
                || x.width() >= 2.0 * pi().upper()) {
            return Interval(-1.0, 1.0);
        }

        double low = std::fmin(function(x.lower()), function(x.upper()));
        double high = std::fmax(function(x.lower()), function(x.upper()));

        const Interval piValue = pi();
        const double first = std::floor(x.lower() / piValue.lower() - offset) - 1.0;
        const double last = std::ceil(x.upper() / piValue.lower() - offset) + 1.0;
        for (double k = first; k <= last; k += 1.0) {
            if ((Interval(k + offset) * piValue).intersects(x)) {
                const double extreme = std::fmod(k, 2.0) == 0.0 ? 1.0 : -1.0;
                low = std::fmin(low, extreme);
                high = std::fmax(high, extreme);
            }
        }

        return Interval(std::fmax(-1.0, detail::downwards(low)), std::fmin(1.0, detail::upwards(high)));
    }

    detail::Backend m_value;
};

class ComplexInterval
{
public:
    ComplexInterval() = default;
    ComplexInterval(const Interval & real, const Interval & imaginary) : m_re(real), m_im(imaginary) {}
    ComplexInterval(std::complex<double> point) : m_re(point.real()), m_im(point.imag()) {}

    const Interval & re() const { return m_re; }
    const Interval & im() const { return m_im; }

    bool containsOrigin() const { return m_re.containsZero() && m_im.containsZero(); }

    friend ComplexInterval operator+(const ComplexInterval & a, const ComplexInterval & b) { return {a.m_re + b.m_re, a.m_im + b.m_im}; }
    friend ComplexInterval operator-(const ComplexInterval & a, const ComplexInterval & b) { return {a.m_re - b.m_re, a.m_im - b.m_im}; }
    friend ComplexInterval operator*(const Interval & scale, const ComplexInterval & z) { return {scale * z.m_re, scale * z.m_im}; }
    friend ComplexInterval operator*(const ComplexInterval & z, const Interval & scale) { return scale * z; }

    Interval magnitude() const { return sqrt(sqr(m_re) + sqr(m_im)); }

    Interval phase() const
    {
        if (containsOrigin()) {
            return Interval(-Interval::pi().upper(), Interval::pi().upper());
        }
        return atan2(m_im, m_re);
    }

private:
    Interval m_re;
    Interval m_im;
};

class PolarInterval
{
public:
    PolarInterval() = default;

    PolarInterval(const Interval & magnitude, const Interval & phase) : m_magnitude(magnitude), m_phase(phase)
    {
        if (magnitude.lower() < 0.0) {
            throw std::domain_error("PolarInterval: a negative magnitude");
        }
    }

    PolarInterval(std::complex<double> point) : PolarInterval(ComplexInterval(point)) {}

    explicit PolarInterval(const ComplexInterval & rectangle)
        : m_magnitude(rectangle.magnitude()), m_phase(rectangle.phase()) {}

    const Interval & magnitude() const { return m_magnitude; }
    const Interval & phase() const { return m_phase; }

    friend PolarInterval operator*(const PolarInterval & a, const PolarInterval & b)
    {
        return PolarInterval(a.m_magnitude * b.m_magnitude, a.m_phase + b.m_phase);
    }

    friend PolarInterval operator/(const PolarInterval & a, const PolarInterval & b)
    {
        return PolarInterval(a.m_magnitude / b.m_magnitude, a.m_phase - b.m_phase);
    }

    friend PolarInterval operator*(const Interval & scale, const PolarInterval & z)
    {
        if (scale.lower() >= 0.0) {
            return PolarInterval(scale * z.m_magnitude, z.m_phase);
        }
        if (scale.upper() <= 0.0) {
            return PolarInterval(-scale * z.m_magnitude, z.m_phase + Interval::pi());
        }
        return PolarInterval(abs(scale) * z.m_magnitude, Interval::hull(z.m_phase, z.m_phase + Interval::pi()));
    }

private:
    Interval m_magnitude;
    Interval m_phase;
};

}

#endif
