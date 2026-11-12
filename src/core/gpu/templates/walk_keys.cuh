/**
 * @file
 * @brief The pieces of preparing a cloud for its walk, as a thread takes
 * them, on the card or on the processor.
 *
 * They are the expressions of walk_cloud.h, one point at a time: the keys
 * of MATLAB's complex order, the projection onto the Nichols plane, the
 * frame and the cells of the neighbour grid, and the reach of epsilon. A
 * double is ordered by the bits of orderedBits(), negative zero taken as
 * zero, so that a radix sort of them is a sort of the values; a cell is
 * its two coordinates in one 64-bit key, the first above the second, which
 * the grid's limit of 1e9 cells a side lets fit.
 */

#ifndef QFTBX_GPU_WALK_KEYS_CUH
#define QFTBX_GPU_WALK_KEYS_CUH

#include <cmath>
#include <cstdint>
#include <cstring>

#include "src/core/gpu/common/device_complex.cuh"
#include "src/core/gpu/common/host_device.h"
#include "src/core/math/constants.h"

namespace qftbx::gpu {

inline constexpr int kCellBits = 30;
inline constexpr std::int64_t kCellReach = 5;
inline constexpr int kCellColumns = 2 * static_cast<int>(kCellReach) + 1;
inline constexpr double kCellsPerEpsilon = 4.0;
inline constexpr double kMaxCells = 1e9;

QFTBX_HD inline std::uint64_t orderedBits(double x)
{
    const double canonical = x + 0.0;
    std::uint64_t bits;
    std::memcpy(&bits, &canonical, sizeof bits);
    return (bits >> 63) != 0 ? ~bits : bits | (std::uint64_t{1} << 63);
}

QFTBX_HD inline bool finite(double x)
{
    return x - x == 0.0;
}

QFTBX_HD inline double magnitudeOf(DeviceComplex z)
{
    return ::hypot(z.real(), z.imag());
}

QFTBX_HD inline double angleOf(DeviceComplex z)
{
    return ::atan2(z.imag(), z.real());
}

QFTBX_HD inline double phaseOf(DeviceComplex z)
{
    return angleOf(z) * 180.0 / qftbx::math::kPi;
}

QFTBX_HD inline DeviceComplex nicholsPoint(DeviceComplex z, double dbPerDegree, bool shiftBelow, double cut)
{
    const double phase = phaseOf(z);
    const double shifted = shiftBelow && phase <= cut ? phase + 360.0 : phase;
    const double magnitude = magnitudeOf(z);
    const double db = magnitude > 0.0 ? 20.0 * ::log10(magnitude) : -400.0;
    return DeviceComplex(shifted, db / dbPerDegree);
}

QFTBX_HD inline bool reaches(DeviceComplex candidate, DeviceComplex centre, double epsilon)
{
    const double dx = candidate.real() - centre.real();
    const double dy = candidate.imag() - centre.imag();

    return dx * dx + dy * dy <= epsilon * epsilon * (1.0 + 1e-9);
}

struct GridFrame
{
    double minRe = 0.0;
    double minIm = 0.0;
    double side = 0.0;
    bool indexed = false;
};

QFTBX_HD inline GridFrame gridFrame(double minRe, double maxRe, double minIm, double maxIm, double epsilon)
{
    GridFrame frame;
    const double side = epsilon / kCellsPerEpsilon;
    const bool bounded = finite(minRe) && finite(maxRe) && finite(minIm) && finite(maxIm);
    frame.indexed = bounded && side > 0 && finite(side) &&
            (maxRe - minRe) / side < kMaxCells && (maxIm - minIm) / side < kMaxCells;
    frame.minRe = minRe;
    frame.minIm = minIm;
    frame.side = side;
    return frame;
}

QFTBX_HD inline std::int64_t cellCoordinate(double value, double minimum, double side)
{
    return static_cast<std::int64_t>(::floor((value - minimum) / side));
}

QFTBX_HD inline std::uint64_t cellKey(std::int64_t x, std::int64_t y)
{
    return (static_cast<std::uint64_t>(x) << kCellBits) | static_cast<std::uint64_t>(y);
}

QFTBX_HD inline std::uint64_t cellKeyOf(const GridFrame & frame, DeviceComplex point)
{
    if (!frame.indexed) {
        return 0;
    }
    return cellKey(cellCoordinate(point.real(), frame.minRe, frame.side),
                   cellCoordinate(point.imag(), frame.minIm, frame.side));
}

}

#endif
