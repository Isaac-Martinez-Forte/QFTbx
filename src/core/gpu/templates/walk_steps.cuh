/**
 * @file
 * @brief The choice of the next point of a walk, one candidate at a time,
 * on the card or on the processor.
 *
 * They are the expressions of the template engine's findSecond() and
 * findNext(): from the start, the angle of the candidate less the half
 * angle the disc of diameter epsilon through both subtends; along the
 * walk, the angle psi from the last point, with the engine's rule for a
 * candidate straight back. The least angle wins, then the farther
 * candidate, then the one first in the cloud's order: a total order, so a
 * warp's threads may weigh their candidates in any order and reduce.
 */

#ifndef QFTBX_GPU_WALK_STEPS_CUH
#define QFTBX_GPU_WALK_STEPS_CUH

#include <cmath>
#include <cstdint>

#include "src/core/gpu/common/device_complex.cuh"
#include "src/core/gpu/common/host_device.h"
#include "src/core/gpu/templates/walk_keys.cuh"
#include "src/core/math/constants.h"

namespace qftbx::gpu {

inline constexpr double kTwoPi = 2 * qftbx::math::kPi;
inline constexpr double kStraightBack = 1e-9;

struct Choice
{
    double angle = 0.0;
    double distance = 0.0;
    std::uint32_t index = 0;
    bool found = false;
};

QFTBX_HD inline bool better(const Choice & a, const Choice & b)
{
    if (a.found != b.found) {
        return a.found;
    }
    if (!a.found) {
        return false;
    }
    if (a.angle != b.angle) {
        return a.angle < b.angle;
    }
    if (a.distance != b.distance) {
        return a.distance > b.distance;
    }
    return a.index < b.index;
}

QFTBX_HD inline Choice secondChoice(DeviceComplex first, DeviceComplex candidate, std::uint32_t index, double epsilon)
{
    Choice choice;
    if (!reaches(candidate, first, epsilon)) {
        return choice;
    }
    const DeviceComplex back = first - candidate;
    const double distance = ::hypot(back.real(), back.imag());
    if (distance > 0 && distance <= epsilon) {
        const DeviceComplex towards = candidate - first;
        double phase = ::atan2(towards.imag(), towards.real());
        if (phase < 0) {
            phase += kTwoPi;
        }
        phase -= ::acos(distance / epsilon);
        choice = {phase, distance, index, true};
    }
    return choice;
}

struct Turn
{
    DeviceComplex previous;
    DeviceComplex current;
    double back;
    double closing;
};

QFTBX_HD inline Turn turnAt(DeviceComplex previous, DeviceComplex current, double epsilon)
{
    const DeviceComplex back = previous - current;
    return {previous, current, ::atan2(back.imag(), back.real()), ::acos(::hypot(back.real(), back.imag()) / epsilon)};
}

QFTBX_HD inline Choice nextChoice(const Turn & turn, DeviceComplex candidate, std::uint32_t index, double epsilon,
                                  bool excludePrevious)
{
    Choice choice;
    if (!reaches(candidate, turn.current, epsilon)) {
        return choice;
    }
    const DeviceComplex towards = candidate - turn.current;
    const double distance = ::hypot(towards.real(), towards.imag());
    if (distance > 0 && distance <= epsilon
            && !(excludePrevious && (candidate == turn.previous || candidate == turn.current))) {
        double phase = ::atan2(towards.imag(), towards.real()) - turn.back;
        if (phase < 0) {
            phase += kTwoPi;
        }
        const double opening = ::acos(distance / epsilon);
        double psi = 0.0;
        if (phase < kStraightBack || phase > kTwoPi - kStraightBack) {
            psi = kTwoPi - opening - turn.closing;
        } else if (phase > 0 && phase < turn.closing) {
            psi = phase + opening - turn.closing;
        } else {
            psi = phase - opening - turn.closing;
        }
        if (psi < 0) {
            psi += kTwoPi;
        }
        choice = {psi, distance, index, true};
    }
    return choice;
}

}

#endif
