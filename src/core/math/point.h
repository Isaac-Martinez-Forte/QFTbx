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
    double phase = 0.0;      //degrees
    double magnitude = 0.0;  //decibels

    NicholsPoint() = default;

    NicholsPoint(double phaseDegrees, double magnitudeDb)
        : phase(phaseDegrees), magnitude(magnitudeDb) {}

    /**
     * @brief Exact equality, coordinate by coordinate.
     *
     * QPointF's was FUZZY (qFuzzyCompare), so this is not the same
     * operator. It is the honest one for a value type, and it is what the
     * one caller wants: BoundaryUnion1D erases the point it just picked out
     * of its own container, so it is looking for that element and not for
     * something near it - which fuzzy equality could have found first among
     * clustered trace points, erasing a different one. The boundary goldens
     * are what confirm the difference does not show up on real data.
     */
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
 * A separate type from NicholsPoint on purpose. The two are both a pair of
 * doubles and mean entirely different things, and the loop viewer used to
 * hold the second in a container of the first: it converted a boundary
 * union to the complex plane and handed it back inside a BoundaryData
 * fabricated for the occasion, whose tell was that it also had to pass
 * empty bucket rows because "this view is only drawn, never classified".
 * With two types the compiler refuses that, which is the whole reason to
 * have two.
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
 * The one place this conversion lives. It used to sit inline in a menu slot
 * of the main window, which is neither where the formula belongs nor where
 * anyone would look for it.
 */
inline NyquistPoint toNyquist(const NicholsPoint & point)
{
    const double magnitude = std::pow(10.0, point.magnitude / 20.0);
    const double radians = point.phase * qftbx::math::kPi / 180.0;

    return NyquistPoint(magnitude * std::cos(radians), magnitude * std::sin(radians));
}

} // namespace qftbx

#endif // QFTBX_POINT_H
