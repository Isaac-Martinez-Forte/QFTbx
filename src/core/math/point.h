/**
 * @file
 * @brief Points of the Nichols chart and of the Nyquist plane.
 *
 * Every curve the toolbox computes lives on the Nichols chart, so a point is
 * a phase in degrees and a magnitude in decibels, named as such. The Nyquist
 * point exists for the conversions the plots need.
 */

#ifndef QFTBX_POINT_H
#define QFTBX_POINT_H

#include <cmath>
#include "src/core/math/constants.h"

namespace qftbx {

/**
 * @brief A point of the Nichols chart: an open-loop phase in DEGREES and a
 * magnitude in DECIBELS.
 *
 * Plain data members rather than accessors, like qftbx::Range: this is an
 * aggregate, and getters over two doubles buy nothing.
 *
 * The names are the point of the type. Every curve the toolbox computes -
 * the boundaries, their union, the traced contours - lives on this chart,
 * and code that reads `point.magnitude > threshold` says what it is doing
 * where `point.y() > threshold` only said what it was made of. The
 * cartesian projection was dropped from the algorithms themselves (the
 * thesis tried detection in the complex plane and discarded it, secs.
 * 4.5-4.6), so in the core there is nothing else a curve point can be.
 */
struct NicholsPoint
{
    double phase = 0.0;   ///< degrees
    double magnitude = 0.0;   ///< decibels

    NicholsPoint() = default;

    NicholsPoint(double phaseDegrees, double magnitudeDb)
        : phase(phaseDegrees), magnitude(magnitudeDb) {}

    /// Exact equality: the one caller erases the element it just picked out of
    /// its own container, and a fuzzy comparison could match a neighbour among
    /// clustered trace points instead.
    bool operator==(const NicholsPoint & other) const
    {
        return phase == other.phase && magnitude == other.magnitude;
    }

    bool operator!=(const NicholsPoint & other) const
    {
        return !(*this == other);
    }
};

/**
 * @brief A point of the complex plane, for the Nyquist view of a loop.
 *
 * A separate type from NicholsPoint on purpose: both are a pair of doubles
 * and they mean entirely different things, so only the type stops a
 * container of one being filled with the other.
 */
struct NyquistPoint
{
    double re = 0.0;
    double im = 0.0;

    NyquistPoint() = default;

    NyquistPoint(double real, double imaginary) : re(real), im(imaginary) {}
};

/**
 * @brief The same loop point read on the complex plane: dB back to a linear
 * magnitude, degrees to radians, then polar to cartesian.
 *
 * The one place this conversion lives.
 */
inline NyquistPoint toNyquist(const NicholsPoint & point)
{
    const double magnitude = std::pow(10.0, point.magnitude / 20.0);
    const double radians = point.phase * qftbx::math::kPi / 180.0;

    return NyquistPoint(magnitude * std::cos(radians), magnitude * std::sin(radians));
}

}

#endif
