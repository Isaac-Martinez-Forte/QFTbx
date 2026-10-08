/**
 * @file
 * @brief Linearly and logarithmically spaced sequences.
 *
 * Each point is computed from its index, so nothing drifts with the count,
 * and the last point is exactly the end requested, as in MATLAB's
 * linspace. A count of zero gives an empty vector, a count of one just the
 * first value, and a descending range is allowed. logspace takes the
 * exponents of its ends, as MATLAB's does.
 */

#ifndef QFTBX_MATH_SEQUENCES_H
#define QFTBX_MATH_SEQUENCES_H

#include <cmath>
#include <cstddef>
#include <vector>

namespace qftbx {
namespace math {

inline std::vector<double> linspace(double first, double last, std::size_t count)
{
    std::vector<double> values;
    values.reserve(count);

    if (count == 0) {
        return values;
    }
    if (count == 1) {
        values.push_back(first);
        return values;
    }

    const double step = (last - first) / static_cast<double>(count - 1);
    for (std::size_t i = 0; i + 1 < count; ++i) {
        values.push_back(first + static_cast<double>(i) * step);
    }
    values.push_back(last);

    return values;
}

inline std::vector<double> logspace(double firstExp, double lastExp, std::size_t count)
{
    std::vector<double> values = linspace(firstExp, lastExp, count);
    for (double& value : values) {
        value = std::pow(10.0, value);
    }
    return values;
}

}
}

#endif
