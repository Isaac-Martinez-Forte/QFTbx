#ifndef QFTBX_RANGE_H
#define QFTBX_RANGE_H

#include <algorithm>

namespace qftbx {

/**
 * @brief A closed real interval [min, max].
 *
 * The toolbox is full of intervals: the uncertainty of a parameter, the
 * phase and magnitude spans of the Nichols grid, a plot's frequency
 * window. They all used to travel as a QPointF whose x was the minimum
 * and whose y was the maximum, which reads as a point and says nothing
 * about which member is which; ordered() also had to be open-coded at
 * every construction site.
 */
struct Range
{
    double min = 0.0;
    double max = 0.0;

    Range() = default;

    Range(double minimum, double maximum) : min(minimum), max(maximum) {}

    /// The same interval with its ends in order (an inverted pair is a
    /// user typo, not a different interval).
    Range ordered() const
    {
        return min <= max ? *this : Range(max, min);
    }

    double width() const
    {
        return max - min;
    }

    /// Midpoint; the natural bisection point of the interval.
    double middle() const
    {
        return min + width() / 2.0;
    }

    bool isDegenerate() const
    {
        return min == max;
    }

    bool contains(double value) const
    {
        return min <= value && value <= max;
    }

    /// True when 'value' lies strictly inside, which is what a cut needs
    /// before it may narrow an interval.
    bool containsStrictly(double value) const
    {
        return min < value && value < max;
    }

    double clamped(double value) const
    {
        return std::min(std::max(value, min), max);
    }

    bool operator==(const Range & other) const
    {
        return min == other.min && max == other.max;
    }

    bool operator!=(const Range & other) const
    {
        return !(*this == other);
    }
};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated to the
//qftbx namespace.
using qftbx::Range;

#endif // QFTBX_RANGE_H
