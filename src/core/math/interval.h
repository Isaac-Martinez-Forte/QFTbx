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

#include <kv/interval.hpp>
#include <kv/rdouble.hpp>

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
 * The arithmetic underneath is kv, Masahide Kashiwagi's verified
 * computation library (3rd-party/kv), in its rounding-emulation mode:
 * the directed roundings come from error-free transformations and the
 * floating-point rounding mode is never touched, so no compiler flag and
 * no thread has to care. The logarithms and the trigonometric functions,
 * which the projection of every controller box and the constraint trees
 * call, take the C library's values widened by a few ulps instead of kv's
 * series (see detail::kLibraryUlps).
 * A domain error (the square root of a negative interval, the logarithm of
 * one that is not positive, a division by an interval containing zero)
 * throws std::domain_error.
 */
namespace qftbx {

namespace detail {

//kv's functions are friends of kv::interval, found by argument-dependent
//lookup only; from inside Interval the name 'width' would find the member.
inline double widthOf(const kv::interval<double> & x) { return width(x); }

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
    Interval() : m_value(0.0) {}

    /// The point interval [value, value].
    Interval(double value) : m_value(value) {}

    /// [lower, upper]; the two may come in either order.
    Interval(double lower, double upper)
        : m_value(lower <= upper ? lower : upper, lower <= upper ? upper : lower) {}

    double lower() const { return m_value.lower(); }
    double upper() const { return m_value.upper(); }

    /// upper - lower, rounded upwards.
    double width() const { return detail::widthOf(m_value); }

    double midpoint() const { return mid(m_value); }

    bool isPoint() const { return m_value.lower() == m_value.upper(); }

    bool contains(double value) const { return m_value.lower() <= value && value <= m_value.upper(); }
    bool contains(const Interval & other) const { return subset(other.m_value, m_value); }
    bool containsZero() const { return zero_in(m_value); }
    bool intersects(const Interval & other) const { return overlap(m_value, other.m_value); }

    /// The common part, or nothing when the two do not meet.
    std::optional<Interval> intersection(const Interval & other) const
    {
        if (!overlap(m_value, other.m_value)) {
            return std::nullopt;
        }
        return Interval(intersect(m_value, other.m_value));
    }

    static Interval hull(const Interval & a, const Interval & b) { return Interval(kv::interval<double>::hull(a.m_value, b.m_value)); }

    /// Enclosures of the constants, not their nearest doubles.
    static Interval pi() { return Interval(kv::constants<kv::interval<double>>::pi()); }
    static Interval e() { return Interval(kv::constants<kv::interval<double>>::e()); }

    friend Interval operator+(const Interval & a, const Interval & b) { return Interval(a.m_value + b.m_value); }
    friend Interval operator-(const Interval & a, const Interval & b) { return Interval(a.m_value - b.m_value); }
    friend Interval operator*(const Interval & a, const Interval & b) { return Interval(a.m_value * b.m_value); }
    friend Interval operator/(const Interval & a, const Interval & b) { return Interval(a.m_value / b.m_value); }
    friend Interval operator-(const Interval & a) { return Interval(-a.m_value); }

    Interval & operator+=(const Interval & b) { m_value += b.m_value; return *this; }
    Interval & operator-=(const Interval & b) { m_value -= b.m_value; return *this; }
    Interval & operator*=(const Interval & b) { m_value *= b.m_value; return *this; }
    Interval & operator/=(const Interval & b) { m_value /= b.m_value; return *this; }

    friend bool operator==(const Interval & a, const Interval & b) { return a.lower() == b.lower() && a.upper() == b.upper(); }
    friend bool operator!=(const Interval & a, const Interval & b) { return !(a == b); }

    friend Interval sqrt(const Interval & x) { return Interval(sqrt(x.m_value)); }

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
    friend Interval abs(const Interval & x) { return Interval(abs(x.m_value)); }
    friend Interval pow(const Interval & x, int n) { return Interval(pow(x.m_value, n)); }
    friend Interval pow(const Interval & x, const Interval & y) { return Interval(pow(x.m_value, y.m_value)); }

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
            return Interval(kv::interval<double>(lo) * lo).hullWith(Interval(kv::interval<double>(hi) * hi));
        }
        if (hi <= 0.0) {
            return Interval(kv::interval<double>(hi) * hi).hullWith(Interval(kv::interval<double>(lo) * lo));
        }

        const Interval a(kv::interval<double>(lo) * lo);
        const Interval b(kv::interval<double>(hi) * hi);
        return Interval(0.0, (a.upper() > b.upper() ? a : b).upper());
    }

    friend std::ostream & operator<<(std::ostream & out, const Interval & x)
    {
        return out << "[" << x.lower() << ", " << x.upper() << "]";
    }

private:
    explicit Interval(const kv::interval<double> & value) : m_value(value) {}

    Interval hullWith(const Interval & other) const { return hull(*this, other); }

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

    kv::interval<double> m_value;
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
