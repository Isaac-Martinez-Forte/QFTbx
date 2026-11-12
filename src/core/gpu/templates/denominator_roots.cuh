/**
 * @file
 * @brief The pieces of finding the poles of a denominator, as a thread
 * takes them, on the card or on the processor.
 *
 * They follow math::polynomialCoefficients and math::polynomialRoots with
 * two changes that keep a warp's threads on the same operation. The
 * transform of the 64 samples is a radix-2 FFT, one butterfly per thread
 * and stage, on the samples placed in bit-reversed order; its twiddles and
 * the points of the unit circle are tables computed once, on the
 * processor, as the processor computes its own. Aberth's iteration moves
 * every root at once from the roots of the previous step (Jacobi), where
 * the processor moves them one after another (Gauss-Seidel); both stop at
 * the same step size. The roots therefore agree with the processor's to
 * rounding, and their count and their frequencies on the imaginary axis
 * are read with the processor's rules.
 */

#ifndef QFTBX_GPU_DENOMINATOR_ROOTS_CUH
#define QFTBX_GPU_DENOMINATOR_ROOTS_CUH

#include <cmath>
#include <cstdint>

#include "src/core/gpu/common/device_complex.cuh"
#include "src/core/gpu/common/host_device.h"
#include "src/core/math/constants.h"

namespace qftbx::gpu {

inline constexpr int kSamples = 64;
inline constexpr int kSampleBits = 6;
inline constexpr int kRecoveredDegree = 24;
inline constexpr int kAberthSteps = 500;

struct RootTables
{
    double unitRe[kSamples];
    double unitIm[kSamples];
    double twiddleRe[kSamples];
    double twiddleIm[kSamples];
};

QFTBX_HD inline int reversedSample(int j)
{
    int reversed = 0;
    for (int bit = 0; bit < kSampleBits; ++bit) {
        reversed |= ((j >> bit) & 1) << (kSampleBits - 1 - bit);
    }
    return reversed;
}

QFTBX_HD inline void butterfly(double * re, double * im, const RootTables & tables, int stage, int b)
{
    const int half = 1 << stage;
    const int i = (b / half) * (2 * half) + (b % half);
    const int k = i + half;
    const int m = (b % half) * (kSamples / (2 * half));
    const double wr = tables.twiddleRe[m];
    const double wi = tables.twiddleIm[m];
    const double tr = wr * re[k] - wi * im[k];
    const double ti = wr * im[k] + wi * re[k];
    re[k] = re[i] - tr;
    im[k] = im[i] - ti;
    re[i] = re[i] + tr;
    im[i] = im[i] + ti;
}

QFTBX_HD inline double recoveredCoefficient(double real, double largest, double radius, int k)
{
    return ::fabs(real) > 1e-9 * largest ? real / ::pow(radius, static_cast<double>(k)) : 0.0;
}

QFTBX_HD inline DeviceComplex aberthStart(double bound, int j, int degree)
{
    const double angle = 2.0 * qftbx::math::kPi * static_cast<double>(j) / static_cast<double>(degree) + 0.4;
    return DeviceComplex(bound * ::cos(angle), bound * ::sin(angle));
}

QFTBX_HD inline DeviceComplex aberthStep(const double * monic, int degree, const double * zRe, const double * zIm,
                                         int j, double bound, double & measure)
{
    const DeviceComplex z(zRe[j], zIm[j]);
    DeviceComplex p(1.0, 0.0);
    DeviceComplex dp(0.0, 0.0);
    for (int k = 1; k <= degree; ++k) {
        dp = dp * z + p;
        p = p * z + monic[k];
    }
    if (dp == DeviceComplex(0.0, 0.0)) {
        measure = 1.0;
        return z + DeviceComplex(1e-8 * bound, 1e-8 * bound);
    }
    const DeviceComplex newton = p / dp;
    DeviceComplex repulsion(0.0, 0.0);
    for (int k = 0; k < degree; ++k) {
        const DeviceComplex term = 1.0 / (z - DeviceComplex(zRe[k], zIm[k]));
        repulsion += k != j ? term : DeviceComplex(0.0, 0.0);
    }
    const DeviceComplex step = newton / (1.0 - newton * repulsion);
    const DeviceComplex moved = z - step;
    measure = cuda::std::abs(step) / (1.0 + cuda::std::abs(moved));
    return moved;
}

QFTBX_HD inline DeviceComplex cleanedRoot(DeviceComplex root)
{
    return ::fabs(root.imag()) <= 1e-10 * (1.0 + ::fabs(root.real())) ? DeviceComplex(root.real(), 0.0) : root;
}

QFTBX_HD inline bool onTheAxis(DeviceComplex root, double largest)
{
    return ::fabs(root.real()) <= 1e-7 * ::fmax(largest, cuda::std::abs(root));
}

QFTBX_HD inline bool inTheRightHalfPlane(DeviceComplex root, double largest)
{
    return !onTheAxis(root, largest) && root.real() > 0.0;
}

QFTBX_HD inline bool onTheAxisAbove(DeviceComplex root, double largest)
{
    return onTheAxis(root, largest) && root.imag() > 1e-9 * largest;
}

}

#endif
