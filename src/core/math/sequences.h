#ifndef QFTBX_MATH_SEQUENCES_H
#define QFTBX_MATH_SEQUENCES_H

#include <cmath>
#include <cstddef>
#include <vector>

namespace qftbx {
namespace math {

/**
 * @brief count values evenly spaced from first to last, both included.
 *
 * Values are computed as first + i*step (no accumulation drift) and the last
 * element is pinned exactly to `last`, matching MATLAB's linspace.
 * count == 0 yields an empty vector; count == 1 yields {first}. A descending
 * range (first > last) is produced naturally.
 */
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

/**
 * @brief count values logarithmically spaced from 10^firstExp to 10^lastExp,
 * both included (the arguments are exponents, matching MATLAB's logspace).
 */
inline std::vector<double> logspace(double firstExp, double lastExp, std::size_t count)
{
    std::vector<double> values = linspace(firstExp, lastExp, count);
    for (double& value : values) {
        value = std::pow(10.0, value);
    }
    return values;
}

} // namespace math
} // namespace qftbx

#endif // QFTBX_MATH_SEQUENCES_H
