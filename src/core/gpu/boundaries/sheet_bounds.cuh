/**
 * @file
 * @brief The arithmetic of a cell of the sheets, as a thread does it, on the
 * card or on the processor.
 *
 * A cell's magnitudes come from three extremes over the template seen from
 * z = -L: the nearest and the farthest distance, and the largest ratio of a
 * radius to a distance (|P0/P| over |P0/P + L| for a sample, the larger
 * radius of a segment's ends over its distance). Without the guard, or for
 * a cloud, they are taken over the samples; for a contour with the guard,
 * the nearest distance and the ratio over the segments of its loops, as
 * SingularLocus::guard does, the farthest over the samples, and the loops'
 * winding numbers say whether z is inside one. Every sample is the first
 * end of a segment, so the samples outside every loop count as segments of
 * length zero, which wind nothing.
 *
 * Each extreme takes two passes. In single precision each sample or
 * segment gives a key that orders it, with a bound on the key's error that
 * covers the rounding of the inputs to single precision, every rounding of
 * the formula, the approximate square root and reciprocal, and underflow,
 * flushed to zero or not (kFloor). The first pass keeps the best bound (the
 * smallest key plus error for the nearest), and the second evaluates in
 * double precision only what can still beat it, so the extreme is the one
 * double precision gives over all of them. The key depends on how far z is:
 * - near the template, the distance d itself: with u = 2^-24 a sample q
 *   gives |d - |q - z|| <= u (|q| + |z| + 6 d), a segment of length l from
 *   a gives u (2 |z| + 2 |a| + 11 l + 14 d);
 * - beyond twice the template's largest radius (kFarField), where d is
 *   about |L| and single precision cannot tell distances apart, the key is
 *   h, with |q + L|^2 = |L|^2 + 2 |L| h and h = q.v + |q|^2 / (2 |L|) for
 *   the direction v of L: its error is a few u |q|, whatever |L|. For a
 *   segment it is the smallest h along it, a quadratic in the position.
 * The constants below are those bounds with room. A segment's step of the
 * winding number is trusted when neither end is level with z and a crossed
 * segment does not pass near it, within the same kind of bound; otherwise
 * it is taken in double precision. Beyond the template's largest radius no
 * loop winds round z. A block whose values could overflow in single
 * precision (kLargest) keeps everything for the double pass.
 *
 * The double precision formulas are the processor's, compared through
 * their squares so that each takes a few products: |q + L| for a sample,
 * SingularLocus's distance to a segment with its inverse squared length
 * kept, and Sunday's crossing count for the winding number, which equals
 * the processor's sum of angles away from the contour. Whether an end of a
 * segment lies below z is decided exactly, on the bits of the doubles read
 * as integers (levelOf), so only a crossed segment that passes near z is
 * left in doubt. The square roots are taken once per cell. Squares hold
 * below 1e150 (kHuge); a sample of a template beyond that is measured by
 * its modulus, scaled by a power of two first. magnitudesOf() turns the extremes into the five closed-loop
 * magnitudes, guard included, and decibels() into the sheets' values; an
 * empty template leaves every value infinite, as the processor's does.
 */

#ifndef QFTBX_GPU_SHEET_BOUNDS_CUH
#define QFTBX_GPU_SHEET_BOUNDS_CUH

#include <cmath>
#include <cstdint>
#include <cstring>

#include <cuda/std/limits>

#include "src/core/gpu/common/device_complex.cuh"
#include "src/core/gpu/common/host_device.h"

namespace qftbx::gpu {

constexpr float kUnit = 5.9604644775390625e-8f;
constexpr float kFloor = 1e-18f;
constexpr float kFloorSquared = 1e-36f;
constexpr float kLargest = 1e18f;
constexpr float kFarField = 2.0f;
constexpr float kPointBase = 2.0f * kUnit;
constexpr float kPointSlope = 8.0f * kUnit;
constexpr float kSegmentBase = 3.0f * kUnit;
constexpr float kSegmentLength = 13.0f * kUnit;
constexpr float kSegmentSlope = 17.0f * kUnit;
constexpr float kWindingBase = 6.0f * kUnit;
constexpr float kFarSquared = 7.0f * kUnit;
constexpr float kFarPoint = 8.0f * kUnit;
constexpr float kFarSegment = 32.0f * kUnit;
constexpr float kRadiusDown = 1.0f - 32.0f * kUnit;
constexpr float kRadiusUp = 1.0f + 32.0f * kUnit;
constexpr float kDistanceDown = 1.0f - 32.0f * kUnit;
constexpr float kDistanceUp = 1.0f + 32.0f * kUnit;
constexpr double kHuge = 1e150;
constexpr float kInfinityF = cuda::std::numeric_limits<float>::infinity();
constexpr double kInfinity = cuda::std::numeric_limits<double>::infinity();

enum class Guard : std::int32_t { None, Cloud, Contour };

struct SampleFloats
{
    float x;
    float y;
    float radius;
};

struct SegmentFloats
{
    std::int64_t levelA;
    std::int64_t levelB;
    float ax;
    float ay;
    float ux;
    float uy;
    float abx;
    float aby;
    float length;
    float alongA;
    float radiusA;
    float largest;
    std::int32_t closes;
};

struct SegmentDoubles
{
    DeviceComplex b;
    double abx;
    double aby;
    double inverseLength2;
    double largest;
};

struct CellView
{
    std::int64_t level;
    float zx;
    float zy;
    float zRadius;
    float vx;
    float vy;
    float half;
    float loopSquared;
};

struct SegmentBases
{
    float error;
    float winding;
    float farSquared;
};

struct Bounds
{
    float nearHi = kInfinityF;
    float farLo = -kInfinityF;
    float ratioLo = 0.0f;
};

struct Flags
{
    bool near;
    bool far;
    bool ratio;
    bool doubtful;
    std::int32_t step;
};

struct Extremes
{
    double nearest2 = kInfinity;
    double farthest2 = 0.0;
    double ratioNumerator = 0.0;
    double ratioDenominator = 1.0;
    std::int32_t nearestIndex = -1;
    bool inside = false;
};

struct Shrink
{
    bool huge = false;
    double factor = 1.0;
};

struct Magnitudes
{
    double stabilityNoise;
    double trackingMin;
    double output;
    double input;
    double effort;
};

QFTBX_HD inline float approxSqrt(float x)
{
#ifdef __CUDA_ARCH__
    float root;
    asm("sqrt.approx.ftz.f32 %0, %1;" : "=f"(root) : "f"(x));
    return root;
#else
    return std::sqrt(x);
#endif
}

QFTBX_HD inline float approxReciprocal(float x)
{
#ifdef __CUDA_ARCH__
    float reciprocal;
    asm("rcp.approx.ftz.f32 %0, %1;" : "=f"(reciprocal) : "f"(x));
    return reciprocal;
#else
    return 1.0f / x;
#endif
}

QFTBX_HD inline std::int64_t levelOf(double y)
{
    const double canonical = y + 0.0;
    std::int64_t bits;
    std::memcpy(&bits, &canonical, sizeof bits);
    return bits < 0 ? bits ^ INT64_C(0x7fffffffffffffff) : bits;
}

QFTBX_HD inline Shrink shrinkOf(double largest)
{
    Shrink shrink;
    shrink.huge = !(largest <= kHuge);
    if (shrink.huge && largest < kInfinity) {
        shrink.factor = ::ldexp(1.0, -::ilogb(largest));
    }
    return shrink;
}

QFTBX_HD inline void prepareSample(DeviceComplex p0, DeviceComplex p, DeviceComplex & q, double & radius,
                                   SampleFloats & single)
{
    q = p0 / p;
    radius = ::hypot(q.real(), q.imag());
    single = {static_cast<float>(q.real()), static_cast<float>(q.imag()), static_cast<float>(radius)};
}

QFTBX_HD inline void prepareSegment(DeviceComplex a, DeviceComplex b, bool closes, SegmentFloats & single,
                                    SegmentDoubles & exact)
{
    const double abx = b.real() - a.real();
    const double aby = b.imag() - a.imag();
    const double length = ::hypot(abx, aby);
    const bool proper = length > 0.0;
    const double radiusA = ::hypot(a.real(), a.imag());
    const double largest = ::fmax(radiusA, ::hypot(b.real(), b.imag()));
    single = {levelOf(a.imag()), levelOf(b.imag()), static_cast<float>(a.real()), static_cast<float>(a.imag()),
              static_cast<float>(proper ? abx / length : 1.0), static_cast<float>(proper ? aby / length : 0.0),
              static_cast<float>(abx), static_cast<float>(aby), static_cast<float>(length),
              static_cast<float>(a.real() * abx + a.imag() * aby), static_cast<float>(radiusA),
              static_cast<float>(largest), closes ? 1 : 0};
    exact = {b, abx, aby, proper ? 1.0 / (abx * abx + aby * aby) : 0.0, largest};
}

QFTBX_HD inline CellView cellView(DeviceComplex L, double loop, double cosine, double sine)
{
    return {levelOf(-L.imag()), static_cast<float>(-L.real()), static_cast<float>(-L.imag()), static_cast<float>(loop),
            static_cast<float>(cosine), static_cast<float>(sine), static_cast<float>(0.5 / loop),
            static_cast<float>(loop * loop)};
}

QFTBX_HD inline bool farFieldOf(float zRadius, float scale)
{
    return zRadius > kFarField * scale;
}

template <bool FarField>
QFTBX_HD inline float sampleBase(const SampleFloats & q, const CellView & c)
{
    if constexpr (FarField) {
        return ::fmaf(kFarPoint, ::fmaf(q.radius * q.radius, c.half, q.radius), kFloor);
    } else {
        return ::fmaf(kPointBase, q.radius + c.zRadius, kFloor);
    }
}

template <bool FarField>
QFTBX_HD inline float sampleKey(const SampleFloats & q, const CellView & c)
{
    if constexpr (FarField) {
        return ::fmaf(q.x, c.vx, ::fmaf(q.y, c.vy, q.radius * q.radius * c.half));
    } else {
        const float dx = q.x - c.zx;
        const float dy = q.y - c.zy;
        return approxSqrt(::fmaf(dx, dx, dy * dy));
    }
}

template <bool FarField>
QFTBX_HD inline float sampleError([[maybe_unused]] float key, float base)
{
    if constexpr (FarField) {
        return base;
    } else {
        return ::fmaf(kPointSlope, key, base);
    }
}

template <bool FarField>
QFTBX_HD inline float distanceHi(float key, [[maybe_unused]] const CellView & c)
{
    if constexpr (FarField) {
        return approxSqrt(::fmaf(2.0f * c.zRadius, key, c.loopSquared)) * kDistanceUp;
    } else {
        return key;
    }
}

template <bool FarField>
QFTBX_HD inline float distanceLo(float key, [[maybe_unused]] const CellView & c)
{
    if constexpr (FarField) {
        return approxSqrt(::fmaxf(::fmaf(2.0f * c.zRadius, key, c.loopSquared), 0.0f)) * kDistanceDown;
    } else {
        return key;
    }
}

template <bool FarField>
QFTBX_HD inline void boundSample(Bounds & b, const SampleFloats & q, float base, const CellView & c, bool near,
                                 bool far, bool ratio)
{
    const float key = sampleKey<FarField>(q, c);
    const float error = sampleError<FarField>(key, base);
    if (near) {
        b.nearHi = ::fminf(b.nearHi, key + error);
    }
    if (far) {
        b.farLo = ::fmaxf(b.farLo, key - error);
    }
    if (ratio) {
        b.ratioLo = ::fmaxf(b.ratioLo, q.radius * kRadiusDown * approxReciprocal(distanceHi<FarField>(key + error, c)));
    }
}

template <bool FarField>
QFTBX_HD inline Flags flagSample(const Bounds & b, const SampleFloats & q, float base, const CellView & c,
                                 bool everything, bool near, bool far, bool ratio)
{
    const float key = sampleKey<FarField>(q, c);
    const float error = sampleError<FarField>(key, base);
    Flags flags{};
    flags.near = near & (everything | (key - error <= b.nearHi));
    flags.far = far & (everything | (key + error >= b.farLo));
    flags.ratio = ratio & (everything | (q.radius * kRadiusUp >= b.ratioLo * distanceLo<FarField>(key - error, c)));
    return flags;
}

template <bool FarField>
QFTBX_HD inline SegmentBases segmentBases(const SegmentFloats & s, const CellView & c)
{
    if constexpr (FarField) {
        return {::fmaf(kFarSegment, ::fmaf(s.largest * s.largest, c.half, s.largest), kFloor), 0.0f, 0.0f};
    } else {
        const float near = c.zRadius + s.radiusA;
        return {::fmaf(kSegmentLength, s.length, ::fmaf(kSegmentBase, near, kFloor)),
                ::fmaf(kWindingBase, c.zRadius + s.largest, kFloor), ::fmaf(kFarSquared * near, near, kFloorSquared)};
    }
}

struct SegmentKeys
{
    float near;
    float nearError;
    float far;
    float farError;
    float perp;
    float ry;
};

template <bool FarField>
QFTBX_HD inline SegmentKeys segmentKeys(const SegmentFloats & s, const SegmentBases & base, const CellView & c)
{
    if constexpr (FarField) {
        const float ha = ::fmaf(s.ax, c.vx, ::fmaf(s.ay, c.vy, s.radiusA * s.radiusA * c.half));
        const float beta = ::fmaf(s.abx, c.vx, ::fmaf(s.aby, c.vy, 2.0f * s.alongA * c.half));
        const float gamma = s.length * s.length * c.half;
        const float t = ::fminf(::fmaxf(-beta * approxReciprocal(2.0f * gamma), 0.0f), 1.0f);
        return {::fmaf(t, ::fmaf(gamma, t, beta), ha), base.error, ha, base.error, 0.0f, 0.0f};
    } else {
        const float rx = c.zx - s.ax;
        const float ry = c.zy - s.ay;
        const float along = ::fmaf(rx, s.ux, ry * s.uy);
        const float perp = ::fmaf(s.ux, ry, -(s.uy * rx));
        const float excess = ::fmaxf(0.0f, ::fmaxf(-along, along - s.length));
        const float distance = approxSqrt(::fmaf(perp, perp, excess * excess));
        return {distance, ::fmaf(kSegmentSlope, distance, base.error), ::fmaf(rx, rx, ry * ry), base.farSquared,
                perp, ry};
    }
}

template <bool FarField>
QFTBX_HD inline void boundSegment(Bounds & b, const SegmentFloats & s, const SegmentBases & base, const CellView & c,
                                  bool far, bool ratio)
{
    const SegmentKeys k = segmentKeys<FarField>(s, base, c);
    b.nearHi = ::fminf(b.nearHi, k.near + k.nearError);
    if (far) {
        b.farLo = ::fmaxf(b.farLo, k.far - k.farError);
    }
    if (ratio) {
        b.ratioLo = ::fmaxf(b.ratioLo,
                            s.largest * kRadiusDown * approxReciprocal(distanceHi<FarField>(k.near + k.nearError, c)));
    }
}

template <bool FarField>
QFTBX_HD inline Flags flagSegment(const Bounds & b, const SegmentFloats & s, const SegmentBases & base,
                                  const CellView & c, bool everything, bool far, bool ratio)
{
    const SegmentKeys k = segmentKeys<FarField>(s, base, c);
    Flags flags{};
    flags.near = everything | (k.near - k.nearError <= b.nearHi);
    flags.far = far & (everything | (k.far + k.farError >= b.farLo));
    flags.ratio = ratio & (everything | (s.largest * kRadiusUp >= b.ratioLo * distanceLo<FarField>(k.near - k.nearError, c)));
    if constexpr (!FarField) {
        const bool aBelow = s.levelA <= c.level;
        const bool bBelow = s.levelB <= c.level;
        const bool up = aBelow & !bBelow;
        const bool down = !aBelow & bBelow;
        flags.step = static_cast<std::int32_t>(up & (k.perp > 0.0f)) - static_cast<std::int32_t>(down & (k.perp < 0.0f));
        flags.doubtful = everything | ((up | down) & (::fabsf(k.perp) <= base.winding));
    }
    return flags;
}

QFTBX_HD inline double square(double x)
{
    return x * x;
}

QFTBX_HD inline std::int32_t crossingExact(DeviceComplex z, DeviceComplex a, DeviceComplex b)
{
    const bool up = a.imag() <= z.imag() && b.imag() > z.imag();
    const bool down = a.imag() > z.imag() && b.imag() <= z.imag();
    const double left = (b.real() - a.real()) * (z.imag() - a.imag()) - (z.real() - a.real()) * (b.imag() - a.imag());
    return (up && left > 0.0) ? 1 : ((down && left < 0.0) ? -1 : 0);
}

QFTBX_HD inline double distance2(DeviceComplex q, DeviceComplex w, const Shrink & shrink)
{
    const double dx = q.real() + w.real();
    const double dy = q.imag() + w.imag();
    return shrink.huge ? square(::hypot(dx, dy) * shrink.factor) : dx * dx + dy * dy;
}

QFTBX_HD inline void keepRatio(Extremes & x, double numerator, double denominator)
{
    const bool larger = numerator * x.ratioDenominator > x.ratioNumerator * denominator;
    x.ratioNumerator = larger ? numerator : x.ratioNumerator;
    x.ratioDenominator = larger ? denominator : x.ratioDenominator;
}

template <bool Indexed>
QFTBX_HD inline void keepSample(Extremes & x, const Flags & f, DeviceComplex q, DeviceComplex w, double radius,
                                [[maybe_unused]] std::int32_t index, const Shrink & shrink)
{
    const double d2 = distance2(q, w, shrink);
    const double radius2 = square(radius * shrink.factor);
    if (f.near && d2 < x.nearest2) {
        x.nearest2 = d2;
        if constexpr (Indexed) {
            x.nearestIndex = index;
        }
    }
    if (f.far && d2 > x.farthest2) {
        x.farthest2 = d2;
    }
    if (f.ratio) {
        keepRatio(x, radius2, d2);
    }
}

template <bool Far>
QFTBX_HD inline void keepSegment(Extremes & x, const Flags & f, DeviceComplex z, DeviceComplex a,
                                 const SegmentDoubles & exact)
{
    const double zax = z.real() - a.real();
    const double zay = z.imag() - a.imag();
    const double t = ::fmin(::fmax((zax * exact.abx + zay * exact.aby) * exact.inverseLength2, 0.0), 1.0);
    const double dx = zax - t * exact.abx;
    const double dy = zay - t * exact.aby;
    const double distance2 = dx * dx + dy * dy;
    if (f.near && distance2 < x.nearest2) {
        x.nearest2 = distance2;
    }
    if (f.ratio) {
        keepRatio(x, exact.largest * exact.largest, distance2);
    }
    if constexpr (Far) {
        const double toA2 = zax * zax + zay * zay;
        if (f.far && toA2 > x.farthest2) {
            x.farthest2 = toA2;
        }
    }
}

QFTBX_HD inline Magnitudes singular()
{
    return {kInfinity, 0.0, kInfinity, kInfinity, kInfinity};
}

QFTBX_HD inline Magnitudes magnitudesOf(const Extremes & x, double loop, double plant, Guard guard, double halfSpacing,
                                        const Shrink & shrink)
{
    const double grow = 1.0 / shrink.factor;
    const double nearest = ::sqrt(x.nearest2) * grow;
    const double farthest = ::sqrt(x.farthest2) * grow;
    const double ratio = ::sqrt(x.ratioNumerator / x.ratioDenominator);
    if (guard == Guard::Contour && (x.inside || !(nearest > 0.0))) {
        return singular();
    }
    Magnitudes m{loop / nearest, loop / farthest, ratio, plant / nearest, loop / plant * ratio};
    if (guard == Guard::Cloud && x.nearestIndex >= 0) {
        const double d = nearest;
        if (!(d > halfSpacing)) {
            return singular();
        }
        const double widen = d / (d - halfSpacing);
        m.stabilityNoise = m.stabilityNoise * widen;
        m.trackingMin = m.trackingMin / widen;
        m.output = m.output * widen;
        m.input = m.input * widen;
        m.effort = m.effort * widen;
    }
    return m;
}

QFTBX_HD inline double violating(double db)
{
    return db != db ? kInfinity : db;
}

struct SheetsUsed
{
    bool stabilityNoise;
    bool tracking;
    bool output;
    bool input;
    bool effort;
};

struct CellValues
{
    double stabilityNoise = kInfinity;
    double tracking = kInfinity;
    double output = kInfinity;
    double input = kInfinity;
    double effort = kInfinity;
};

QFTBX_HD inline CellValues decibels(const Magnitudes & m, const SheetsUsed & used)
{
    CellValues values;
    const double stability = (used.stabilityNoise || used.tracking) ? 20 * ::log10(m.stabilityNoise) : 0.0;
    if (used.stabilityNoise) {
        values.stabilityNoise = violating(stability);
    }
    if (used.tracking) {
        values.tracking = violating(stability - (20 * ::log10(m.trackingMin)));
    }
    if (used.output) {
        values.output = violating(20 * ::log10(m.output));
    }
    if (used.input) {
        values.input = violating(20 * ::log10(m.input));
    }
    if (used.effort) {
        values.effort = violating(20 * ::log10(m.effort));
    }
    return values;
}

}

#endif
