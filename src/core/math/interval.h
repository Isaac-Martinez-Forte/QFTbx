/**
 * @file
 * @brief The interval arithmetic of the toolbox.
 *
 * Three types, and the only ones the rest of the code sees:
 *
 * - Interval: a closed real interval [lower, upper], with the four
 *   operations rounded outwards and the elementary functions enclosed
 *   rigorously. It is what the HC4 filter of algorithm MR propagates and
 *   what every magnitude and phase is made of.
 * - ComplexInterval: a rectangle of the complex plane, Re and Im
 *   intervals. Sums and scalar products are exact in shape here; its
 *   magnitude() and phase() are rigorous readings of the rectangle.
 * - PolarInterval: a magnitude interval and a phase interval. Products and
 *   quotients are exact in shape here, which is why the loop transmission
 *   L0 = k P0 prod(jw + z) / prod(jw + p) is assembled in polar form: the
 *   thesis (section 1.2.5) writes the natural interval extension as sums
 *   of 20 log|.| and sums of atan(w/.), and multiplying rectangles would
 *   inflate the enclosure with every factor.
 *
 * The arithmetic underneath is one of two libraries, chosen at
 * configuration time (QFTBX_INTERVAL_BACKEND) and confined to
 * detail::Backend: kv, Masahide Kashiwagi's verified computation library
 * (3rd-party/kv), by default, in its rounding-emulation mode, where the
 * directed roundings come from error-free transformations and the
 * floating-point rounding mode is never touched; or C-XSC, fetched from
 * its repository, which switches the rounding mode around each operation
 * and needs -frounding-math. The backend supplies the four operations,
 * the square root, the integer power and pi; everything else is written
 * here over those, so both give the same enclosures up to rounding, and a
 * disagreement between them beyond that is a bug in one of them. kv's
 * functions are friends of kv::interval, found by argument-dependent lookup
 * only, hence the wrappers in detail.
 *
 * The exponential, the logarithms, the trigonometric functions and their
 * inverses, which the projection of every controller box and the constraint
 * trees of MR call for every factor of every box, take the C library's
 * values widened by detail::kLibraryUlps ulps on each side instead of either
 * library's series, whose enclosures cost some microseconds each where the
 * library takes nanoseconds. glibc's table of known maximum errors (manual,
 * "Known Maximum Errors in Math Functions") lists at most two ulps for these
 * functions on x86_64, with the stated goal of results "within a few ulp";
 * four ulps double the largest listed bound, and four ulps of a phase in
 * radians or of a magnitude in dB are far below anything the algorithms
 * resolve. The widening is an integer step on the magnitude of the bit
 * pattern, since the doubles are ordered as their bit patterns read as
 * sign-and-magnitude integers; values too close to zero to take the step,
 * and non-finite ones, go through nextafter.
 *
 * The monotone functions take the library's values at the ends: the
 * exponential, whose overflowing end is infinite, the logarithms, the arc
 * tangent, asin and acos. sin and cos add the extremes +1 and -1 wherever a
 * maximum or a minimum lies inside; the extremes sit at (k + 1/2) pi and at
 * k pi, each located with the enclosure of pi, so a rounding near one can
 * only add an extreme, never miss it. The candidates k run from
 * floor(lower / pi - offset) to ceil(upper / pi - offset), which the rounding
 * of the quotient cannot move by more than one for arguments below
 * detail::kLargestReducedArgument; beyond it the position of a multiple of
 * pi is not resolved well enough, and the whole range is returned. tan is
 * monotone between its poles and the whole real line when the interval may
 * hold one. sqr is tight, [0, max] where x straddles zero, where x * x would
 * give a negative lower end; on one side of zero it is the square of the end
 * nearer zero rounded down and that of the farther rounded up, the two of the
 * four directed products that the hull of the squares of the ends would keep. pow(x, y) is exp(y log x), for a strictly
 * positive x. atan2(y, x) is the argument of the rectangle {x + j y}: the
 * whole turn [-pi, pi] when the rectangle contains the origin; otherwise the
 * argument is continuous over it and monotone along each edge, so its
 * extremes sit at the corners, each asked once: a side whose two ends have
 * the same bits, as the factor jw + z of a real zero has, is one end, and a
 * point is one corner, with +0 and -0 told apart. A rectangle crossing the
 * negative real axis is measured from that axis and turned by pi, so the
 * result runs continuously past pi instead of splitting at the cut. pi() and e() are
 * enclosures of the constants, not their nearest doubles; an Interval given
 * its ends in either order takes them in order, and width() is rounded
 * upwards.
 *
 * A domain error throws std::domain_error in both backends: the square root
 * of an interval reaching below zero, the logarithm of one that is not
 * strictly positive, asin or acos of one leaving [-1, 1], a division by an
 * interval containing zero, a negative integer power of one, and a negative
 * magnitude given to a PolarInterval. The magnitude of a ComplexInterval
 * runs from the distance of the origin to the rectangle to the distance to
 * its farthest corner, and its phase is continuous: it may reach past pi
 * when the rectangle crosses the negative real axis and is the whole turn
 * when it contains the origin. The phase of a PolarInterval is not reduced
 * modulo a turn: a product adds phases as they come, so a long product can
 * span more than 2 pi, and the caller maps it onto whatever branch it works
 * on. A negative scale turns the phase by pi, and a scale straddling zero
 * takes the hull of the phase and the phase turned by pi.
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
