/**
 * @file
 * @brief Solving a g^2 + b g + c >= 0 on g >= 0 by cases.
 */

#include "src/core/math/quadratic_set.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace qftbx {
namespace math {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

}

RangeUnion whereNonNegative(double a, double b, double c)
{
    const RangeUnion positive = RangeUnion::of(0.0, kInfinity);

    if (a == 0.0) {
        if (b == 0.0) {
            return c >= 0.0 ? positive : RangeUnion();
        }
        const double root = -c / b;
        return b > 0.0 ? RangeUnion::of(std::max(0.0, root), kInfinity)
                       : RangeUnion::of(0.0, root);
    }

    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) {
        return a > 0.0 ? positive : RangeUnion();
    }

    const double s = std::sqrt(discriminant);
    const double qv = -0.5 * (b + (b >= 0.0 ? s : -s));
    double r1 = 0.0, r2 = 0.0;
    if (qv != 0.0) {
        r1 = qv / a;
        r2 = c / qv;
    }
    if (r1 > r2) {
        std::swap(r1, r2);
    }

    if (a > 0.0) {
        if (r1 > 0.0) {
            const double lower[2] = {0.0, std::max(0.0, r2)};
            const double upper[2] = {r1, kInfinity};
            return RangeUnion::of(lower, upper, 2);
        }
        return RangeUnion::of(std::max(0.0, r2), kInfinity);
    }
    return RangeUnion::of(std::max(0.0, r1), r2);
}

}
}
