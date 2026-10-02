/**
 * @file
 * @brief Solving a g^2 + b g + c >= 0, and its complement, on g >= 0 by cases.
 */

#include "src/core/math/quadratic_set.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace qftbx {
namespace math {

namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

std::pair<double, double> stableRoots(double a, double b, double c, double discriminant)
{
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
    return {r1, r2};
}

}

void whereNonNegative(double a, double b, double c, RangeUnion & set)
{
    if (a == 0.0) {
        if (b == 0.0) {
            if (c >= 0.0) {
                set.assign(0.0, kInfinity);
            } else {
                set.clear();
            }
            return;
        }
        const double root = -c / b;
        if (b > 0.0) {
            set.assign(std::max(0.0, root), kInfinity);
        } else {
            set.assign(0.0, root);
        }
        return;
    }

    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) {
        if (a > 0.0) {
            set.assign(0.0, kInfinity);
        } else {
            set.clear();
        }
        return;
    }

    const auto [r1, r2] = stableRoots(a, b, c, discriminant);

    if (a > 0.0) {
        if (r1 > 0.0) {
            const double lower[2] = {0.0, std::max(0.0, r2)};
            const double upper[2] = {r1, kInfinity};
            set.assign(lower, upper, 2);
            return;
        }
        set.assign(std::max(0.0, r2), kInfinity);
        return;
    }
    set.assign(std::max(0.0, r1), r2);
}

RangeUnion whereNonNegative(double a, double b, double c)
{
    RangeUnion set;
    whereNonNegative(a, b, c, set);
    return set;
}

void appendWhereNegative(double a, double b, double c, std::vector<double> & lower, std::vector<double> & upper)
{
    if (a == 0.0) {
        if (b == 0.0) {
            if (c < 0.0) {
                lower.push_back(0.0);
                upper.push_back(kInfinity);
            }
            return;
        }
        const double root = -c / b;
        if (b > 0.0) {
            if (root > 0.0) {
                lower.push_back(0.0);
                upper.push_back(root);
            }
        } else {
            lower.push_back(std::max(0.0, root));
            upper.push_back(kInfinity);
        }
        return;
    }

    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0) {
        if (a < 0.0) {
            lower.push_back(0.0);
            upper.push_back(kInfinity);
        }
        return;
    }

    const auto [r1, r2] = stableRoots(a, b, c, discriminant);
    if (a > 0.0) {
        if (r2 > 0.0) {
            lower.push_back(std::max(0.0, r1));
            upper.push_back(r2);
        }
    } else {
        if (r1 > 0.0) {
            lower.push_back(0.0);
            upper.push_back(r1);
        }
        lower.push_back(std::max(0.0, r2));
        upper.push_back(kInfinity);
    }
}

}
}
