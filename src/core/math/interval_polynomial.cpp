/**
 * @file
 * @brief Interval polynomial arithmetic and the interval Routh table.
 */

#include "src/core/math/interval_polynomial.h"

#include <algorithm>
#include <cstddef>

namespace qftbx {
namespace math {

std::vector<Interval> intervalPolynomialProduct(const std::vector<Interval> & a, const std::vector<Interval> & b)
{
    if (a.empty()) {
        return b;
    }
    if (b.empty()) {
        return a;
    }
    std::vector<Interval> product(a.size() + b.size() - 1, Interval(0.0));
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < b.size(); ++j) {
            product[i + j] += a[i] * b[j];
        }
    }
    return product;
}

std::vector<Interval> intervalPolynomialSum(const std::vector<Interval> & a, const std::vector<Interval> & b)
{
    const std::vector<Interval> & longer = a.size() >= b.size() ? a : b;
    const std::vector<Interval> & shorter = a.size() >= b.size() ? b : a;
    std::vector<Interval> sum = longer;
    const std::size_t offset = longer.size() - shorter.size();
    for (std::size_t i = 0; i < shorter.size(); ++i) {
        sum[offset + i] += shorter[i];
    }
    return sum;
}

bool provablyNotHurwitz(const std::vector<Interval> & coefficients)
{
    std::vector<Interval> a = coefficients;
    while (!a.empty() && a.front().lower() == 0.0 && a.front().upper() == 0.0) {
        a.erase(a.begin());
    }
    if (a.size() < 2) {
        return false;
    }

    if (a.front().upper() < 0.0) {
        for (Interval & c : a) {
            c = -c;
        }
    } else if (!(a.front().lower() > 0.0)) {
        return false;
    }

    for (const Interval & c : a) {
        if (c.upper() <= 0.0) {
            return true;
        }
    }

    const std::size_t degree = a.size() - 1;
    const std::size_t width = (degree + 2) / 2;
    std::vector<Interval> previous(width, Interval(0.0));
    std::vector<Interval> current(width, Interval(0.0));
    for (std::size_t i = 0; i * 2 < a.size(); ++i) {
        previous[i] = a[i * 2];
    }
    for (std::size_t i = 0; i * 2 + 1 < a.size(); ++i) {
        current[i] = a[i * 2 + 1];
    }

    std::vector<Interval> next(width, Interval(0.0));
    for (std::size_t row = 2; row <= degree; ++row) {
        if (current.front().upper() <= 0.0) {
            return true;
        }
        if (!(current.front().lower() > 0.0)) {
            return false;
        }
        for (std::size_t i = 0; i + 1 < width; ++i) {
            next[i] = (current[0] * previous[i + 1] - previous[0] * current[i + 1]) / current[0];
        }
        next.back() = Interval(0.0);
        previous.swap(current);
        current.swap(next);
    }

    return current.front().upper() <= 0.0;
}

}
}
