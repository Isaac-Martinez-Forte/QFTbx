/**
 * @file
 * @brief A closed interval of reals as a named pair.
 *
 * The uncertainty of a parameter, the spans of the Nichols grid and a plot's
 * frequency window are all ranges. The pair says which end is which, orders
 * an inverted pair and gives the width and midpoint every bisection needs.
 * An inverted pair is a typo for the same interval, not another one, and
 * containsStrictly() is what a cut asks before it may narrow a range.
 */

#ifndef QFTBX_RANGE_H
#define QFTBX_RANGE_H

#include <algorithm>

namespace qftbx {

struct Range
{
    double min = 0.0;
    double max = 0.0;

    Range() = default;

    Range(double minimum, double maximum) : min(minimum), max(maximum) {}

    Range ordered() const
    {
        return min <= max ? *this : Range(max, min);
    }

    double width() const
    {
        return max - min;
    }

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

}

#endif
