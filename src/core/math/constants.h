#ifndef QFTBX_MATH_CONSTANTS_H
#define QFTBX_MATH_CONSTANTS_H

#include "src/core/math/constants.h"

namespace qftbx {
namespace math {

/**
 * @brief The mathematical constants the toolbox uses, spelled once.
 *
 * M_PI and M_E are POSIX, not C++: they came from <cmath> only because the
 * GNU headers define them, and thirteen files depended on that. C++20 has
 * std::numbers::pi and std::numbers::e in <numbers>, which the compiler this
 * builds with (gcc 8.5) does not ship yet; when it does, these two lines are
 * the only ones to change.
 */
inline constexpr double kPi = 3.141592653589793238462643383279502884;
inline constexpr double kE = 2.718281828459045235360287471352662498;

/// Degrees in one radian (180 / pi), for the phase conversions of the
/// Nichols plane. Written out: the build runs with -frounding-math for
/// C-XSC, under which a floating division is not a constant expression.
inline constexpr double kDegreesPerRadian = 57.295779513082320876798154814105;

} // namespace math
} // namespace qftbx

#endif // QFTBX_MATH_CONSTANTS_H
