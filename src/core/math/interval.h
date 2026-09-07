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

/**
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
 * disagreement between them beyond that is a bug in one of them.
 *
 * The exponential, the logarithms and the trigonometric functions, which
 * the projection of every controller box and the constraint trees call,
 * take the C library's values widened by a few ulps instead of either
 * library's series (see detail::kLibraryUlps). A domain error (the square
 * root of a negative interval, the logarithm of one that is not positive,
 * a division by an interval containing zero) throws std::domain_error in
 * both backends.
 */
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

#else

using Backend = kv::interval<double>;

inline Backend backend(double lower, double upper) { return kv::interval<double>(lower, upper); }
inline double lowerOf(const Backend & x) { return x.lower(); }
inline double upperOf(const Backend & x) { return x.upper(); }
//kv's functions are friends of kv::interval, found by argument-dependent
//lookup only, hence the wrappers.
inline double widthOf(const Backend & x) { return width(x); }
inline Backend sqrtOf(const Backend & x) { return sqrt(x); }
inline Backend powerOf(const Backend & x, int n) { return pow(x, n); }
inline Backend piOf() { return kv::constants<kv::interval<double>>::pi(); }

#endif

//The exponential, the logarithms, the trigonometric functions and their
//inverses come from the C library, widened by this many ulps on each side. The projection of a
//controller box and the constraint trees of algorithm MR call them for
//every factor of every box, and kv's own series enclosures cost some
//microseconds each where the library takes nanoseconds. glibc's table of
//known maximum errors (manual, "Known Maximum Errors in Math Functions")
//lists at most two ulps for these functions on x86_64, with the stated goal
//of results "within a few ulp"; four ulps double the largest listed bound,
//and four ulps of a phase in radians or of a magnitude in dB are far below
//anything the algorithms resolve.
constexpr int kLibraryUlps = 4;

//Beyond this magnitude the position of a multiple of pi is not resolved
//well enough to locate the extremes of sin and cos; the whole range is
//returned instead.
constexpr double kLargestReducedArgument = 1e15;

//A finite double moved kLibraryUlps representable values away from zero
//or towards it: the ordering of the doubles is the ordering of their bit
//patterns as sign-and-magnitude integers, so the move is an integer step on
//the magnitude. Values too close to zero to take the step, and non-finite
//ones, go through nextafter.
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

} // namespace detail

class Interval
{
public:
    /// The point interval [0, 0].
    Interval() : m_value(detail::backend(0.0, 0.0)) {}

    /// The point interval [value, value].
    Interval(double value) : m_value(detail::backend(value, value)) {}

    /// [lower, upper]; the two may come in either order.
    Interval(double lower, double upper)
        : m_value(detail::backend(lower <= upper ? lower : upper, lower <= upper ? upper : lower)) {}

    double lower() const { return detail::lowerOf(m_value); }
    double upper() const { return detail::upperOf(m_value); }

    /// upper - lower, rounded upwards.
    double width() const { return detail::widthOf(m_value); }

    double midpoint() const { return lower() + (upper() - lower()) / 2.0; }

    bool isPoint() const { return lower() == upper(); }

    bool contains(double value) const { return lower() <= value && value <= upper(); }
    bool contains(const Interval & other) const { return lower() <= other.lower() && other.upper() <= upper(); }
    bool containsZero() const { return contains(0.0); }
    bool intersects(const Interval & other) const { return lower() <= other.upper() && other.lower() <= upper(); }

    /// The common part, or nothing when the two do not meet.
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

    /// Enclosures of the constants, not their nearest doubles.
    static Interval pi() { return Interval(detail::piOf()); }
    static Interval e() { return exp(Interval(1.0)); }

    friend Interval operator+(const Interval & a, const Interval & b) { return Interval(a.m_value + b.m_value); }
    friend Interval operator-(const Interval & a, const Interval & b) { return Interval(a.m_value - b.m_value); }
    friend Interval operator*(const Interval & a, const Interval & b) { return Interval(a.m_value * b.m_value); }
    friend Interval operator-(const Interval & a) { return Interval(-a.m_value); }

    /// Throws std::domain_error when the divisor contains zero.
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

    /// Throws std::domain_error when x reaches below zero.
    friend Interval sqrt(const Interval & x)
    {
        if (x.lower() < 0.0) {
            throw std::domain_error("sqrt: the interval reaches below zero");
        }
        return Interval(detail::sqrtOf(x.m_value));
    }

    /// The exponential, monotone: the C library's values at the ends,
    /// widened; an overflowing end is infinite.
    friend Interval exp(const Interval & x)
    {
        return Interval(detail::downwards(std::exp(x.lower())), detail::upwards(std::exp(x.upper())));
    }

    /// The logarithms and the arc tangent are monotone: the C library's
    /// values at the ends, widened (see detail::kLibraryUlps). A logarithm
    /// of an interval that is not strictly positive throws
    /// std::domain_error.
    friend Interval log(const Interval & x) { return positiveMonotone(x, std::log, "log"); }
    friend Interval log10(const Interval & x) { return positiveMonotone(x, std::log10, "log10"); }
    friend Interval log2(const Interval & x) { return positiveMonotone(x, std::log2, "log2"); }
    friend Interval atan(const Interval & x)
    {
        return Interval(detail::downwards(std::atan(x.lower())), detail::upwards(std::atan(x.upper())));
    }

    /// sin and cos: the library's values at the ends, widened, plus the
    /// extremes +1 and -1 wherever a maximum or a minimum lies inside. The
    /// extremes sit at (k + 1/2) pi and at k pi; each is located with the
    /// enclosure of pi, so a rounding near one can only add an extreme,
    /// never miss it.
    friend Interval sin(const Interval & x) { return periodic(x, std::sin, 0.5); }
    friend Interval cos(const Interval & x) { return periodic(x, std::cos, 0.0); }

    /// tan: monotone between its poles; the whole real line when the
    /// interval may contain a pole (k + 1/2) pi.
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

    /// asin and acos on [-1, 1]; an interval reaching outside throws
    /// std::domain_error.
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

    /// x^n; a negative n with x containing zero throws std::domain_error.
    friend Interval pow(const Interval & x, int n)
    {
        if (n < 0 && x.containsZero()) {
            throw std::domain_error("pow: a negative power of an interval containing zero");
        }
        return Interval(detail::powerOf(x.m_value, n));
    }

    /// x^y as exp(y log x); x must be strictly positive.
    friend Interval pow(const Interval & x, const Interval & y) { return exp(y * log(x)); }

    /// atan2(y, x): the argument of the point set {x + j y}, a rectangle.
    /// The whole turn [-pi, pi] when the rectangle contains the origin;
    /// otherwise the argument is continuous over the rectangle and monotone
    /// along each edge, so its extremes sit at the corners. A rectangle
    /// crossing the negative real axis is measured from that axis and turned
    /// by pi, so the result runs continuously past pi instead of splitting at
    /// the cut.
    friend Interval atan2(const Interval & y, const Interval & x)
    {
        if (x.containsZero() && y.containsZero()) {
            return Interval(-pi().upper(), pi().upper());
        }

        const bool acrossTheNegativeAxis = x.upper() < 0.0 && y.containsZero();
        double lowest = std::numeric_limits<double>::infinity();
        double highest = -std::numeric_limits<double>::infinity();

        for (const double xi : {x.lower(), x.upper()}) {
            for (const double yi : {y.lower(), y.upper()}) {
                const double angle = acrossTheNegativeAxis ? std::atan2(-yi, -xi) : std::atan2(yi, xi);
                lowest = std::fmin(lowest, angle);
                highest = std::fmax(highest, angle);
            }
        }

        const Interval corners(detail::downwards(lowest), detail::upwards(highest));
        return acrossTheNegativeAxis ? pi() + corners : corners;
    }

    /// x^2, tight: [0, max] when x straddles zero, where x * x would give
    /// [-|lo||hi|, ...].
    friend Interval sqr(const Interval & x)
    {
        const double lo = x.lower();
        const double hi = x.upper();

        if (lo >= 0.0) {
            return hull(Interval(lo) * Interval(lo), Interval(hi) * Interval(hi));
        }
        if (hi <= 0.0) {
            return hull(Interval(hi) * Interval(hi), Interval(lo) * Interval(lo));
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

    //The smallest k such that (k + offset) pi may lie inside x, judged with
    //the enclosure of pi, or nothing when no such point can be inside. The
    //candidates are the indices from floor(lower / pi - offset) to
    //ceil(upper / pi - offset), which the rounding of the quotient cannot
    //move by more than one for arguments below kLargestReducedArgument.
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

        //Consecutive extremes alternate in sign: +1 at even k for both
        //cos (k pi) and sin ((k + 1/2) pi).
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

/**
 * @brief A rectangle of the complex plane: a real interval and an
 * imaginary one.
 */
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

    /// |z| over the rectangle: from the distance of the origin to the
    /// rectangle to the distance to its farthest corner.
    Interval magnitude() const { return sqrt(sqr(m_re) + sqr(m_im)); }

    /// arg z over the rectangle, continuous: an interval that may reach
    /// past pi when the rectangle crosses the negative real axis, and the
    /// whole turn [-pi, pi] when it contains the origin.
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

/**
 * @brief A set of complex numbers given by its magnitudes and its phases:
 * the form in which products and quotients keep their shape.
 *
 * The phase is not reduced modulo a turn: a product adds phases as they
 * come, so a long product can span more than 2 pi, and the caller maps it
 * onto whatever branch it works on.
 */
class PolarInterval
{
public:
    PolarInterval() = default;

    /// magnitude must not reach below zero.
    PolarInterval(const Interval & magnitude, const Interval & phase) : m_magnitude(magnitude), m_phase(phase)
    {
        if (magnitude.lower() < 0.0) {
            throw std::domain_error("PolarInterval: a negative magnitude");
        }
    }

    /// The point z.
    PolarInterval(std::complex<double> point) : PolarInterval(ComplexInterval(point)) {}

    /// The rectangle read in polar coordinates.
    explicit PolarInterval(const ComplexInterval & rectangle)
        : m_magnitude(rectangle.magnitude()), m_phase(rectangle.phase()) {}

    const Interval & magnitude() const { return m_magnitude; }
    const Interval & phase() const { return m_phase; }

    friend PolarInterval operator*(const PolarInterval & a, const PolarInterval & b)
    {
        return PolarInterval(a.m_magnitude * b.m_magnitude, a.m_phase + b.m_phase);
    }

    /// Throws std::domain_error when the divisor's magnitude reaches zero.
    friend PolarInterval operator/(const PolarInterval & a, const PolarInterval & b)
    {
        return PolarInterval(a.m_magnitude / b.m_magnitude, a.m_phase - b.m_phase);
    }

    friend PolarInterval operator*(const Interval & scale, const PolarInterval & z)
    {
        //A real scale keeps the phase when it is non-negative and turns it
        //by pi when negative; one straddling zero contributes both.
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

} // namespace qftbx

#endif // QFTBX_MATH_INTERVAL_H
