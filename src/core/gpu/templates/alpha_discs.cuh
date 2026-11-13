/**
 * @file
 * @brief The test of an edge of the alpha-shape, one point at a time, on the
 * card or on the processor.
 *
 * They are the expressions of alphaShape(): the two discs of diameter
 * epsilon through both ends of a chord, their centres on either side of
 * its midpoint, and whether a point lies inside one, a point within 1e-9 of
 * the circle not blocking it. A chord of no length has no discs. The angle
 * of a neighbour is the one the faces are walked in.
 */

#ifndef QFTBX_GPU_ALPHA_DISCS_CUH
#define QFTBX_GPU_ALPHA_DISCS_CUH

#include <cmath>

#include "src/core/gpu/common/device_complex.cuh"
#include "src/core/gpu/common/host_device.h"

namespace qftbx::gpu {

struct Discs
{
    DeviceComplex first;
    DeviceComplex second;
    double blocking = 0.0;
    bool chord = false;
};

QFTBX_HD inline Discs discsOf(DeviceComplex a, DeviceComplex b, double epsilon)
{
    Discs discs;
    const double radius = epsilon / 2.0;
    discs.blocking = radius * radius * (1.0 - 1e-9);
    const DeviceComplex chord = b - a;
    const double length = ::hypot(chord.real(), chord.imag());
    const double half = length / 2.0;
    discs.chord = half > 0.0;
    if (!discs.chord) {
        return discs;
    }
    const DeviceComplex mid((a.real() + b.real()) / 2.0, (a.imag() + b.imag()) / 2.0);
    const double square = radius * radius - half * half;
    const double sagitta = ::sqrt(0.0 < square ? square : 0.0);
    const DeviceComplex unit(chord.real() / length, chord.imag() / length);
    const DeviceComplex normal(-unit.imag(), unit.real());
    discs.first = mid + DeviceComplex(sagitta * normal.real(), sagitta * normal.imag());
    discs.second = mid + DeviceComplex(-sagitta * normal.real(), -sagitta * normal.imag());
    return discs;
}

QFTBX_HD inline bool blocks(DeviceComplex point, DeviceComplex centre, double blocking)
{
    const DeviceComplex d = point - centre;
    return d.real() * d.real() + d.imag() * d.imag() < blocking;
}

QFTBX_HD inline double angleTowards(DeviceComplex from, DeviceComplex to)
{
    const DeviceComplex d = to - from;
    return ::atan2(d.imag(), d.real());
}

}

#endif
