#ifndef QFTBX_MATH_CONSTANTS_H
#define QFTBX_MATH_CONSTANTS_H

#if __has_include(<numbers>)
#include <numbers>
#endif

namespace qftbx {
namespace math {

/// pi and e, from the standard library where it provides them and spelled
/// out where it does not yet.
#if __has_include(<numbers>)
inline constexpr double kPi = std::numbers::pi;
inline constexpr double kE = std::numbers::e;
#else
inline constexpr double kPi = 3.141592653589793238462643383279502884;
inline constexpr double kE = 2.718281828459045235360287471352662498;
#endif

} // namespace math
} // namespace qftbx

#endif // QFTBX_MATH_CONSTANTS_H
