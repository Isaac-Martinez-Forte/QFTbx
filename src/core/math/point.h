/**
 * @file
 * @brief Points of the Nichols chart and of the Nyquist plane.
 *
 * Every curve the toolbox computes lives on the Nichols chart, so a point is
 * a phase in degrees and a magnitude in decibels, named as such, plain data
 * members of an aggregate. The Nyquist point exists for the conversions the
 * plots need, a separate type although both are two doubles so that a
 * container of one cannot be filled with the other; toNyquist() is the one
 * place the conversion lives (dB to linear, degrees to radians, polar to
 * cartesian). NicholsPoint compares exactly, on purpose: its caller erases
 * the very element it picked out of its own container, and a tolerance
 * could match a neighbour among clustered trace points instead.
 */

#ifndef QFTBX_POINT_H
#define QFTBX_POINT_H

#include <cmath>
#include "src/core/math/constants.h"

namespace qftbx {

struct NicholsPoint
{
    double phase = 0.0;
    double magnitude = 0.0;

    NicholsPoint() = default;

    NicholsPoint(double phaseDegrees, double magnitudeDb)
        : phase(phaseDegrees), magnitude(magnitudeDb) {}

    bool operator==(const NicholsPoint & other) const
    {
        return phase == other.phase && magnitude == other.magnitude;
    }

    bool operator!=(const NicholsPoint & other) const
    {
        return !(*this == other);
    }
};

struct NyquistPoint
{
    double re = 0.0;
    double im = 0.0;

    NyquistPoint() = default;

    NyquistPoint(double real, double imaginary) : re(real), im(imaginary) {}
};

inline NyquistPoint toNyquist(const NicholsPoint & point)
{
    const double magnitude = std::pow(10.0, point.magnitude / 20.0);
    const double radians = point.phase * qftbx::math::kPi / 180.0;

    return NyquistPoint(magnitude * std::cos(radians), magnitude * std::sin(radians));
}

}

#endif
