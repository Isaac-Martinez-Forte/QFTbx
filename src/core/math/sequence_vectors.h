/**
 * @file
 * @brief Sequence generators in the signature the algorithms call.
 *
 * Wrappers over the spaced sequences of the math library, plus a float
 * variant kept for the GPU path.
 */

#ifndef QFTBX_MATH_SEQUENCE_VECTORS_H
#define QFTBX_MATH_SEQUENCE_VECTORS_H

#include <cstdint>
#include <vector>

namespace qftbx {

/// Wrappers over qftbx::math (src/core/math/sequences.h).
std::vector <double> linspace(double a, double b, std::int32_t N);
std::vector <double> logspace (double a, double b, std::int32_t N);

/// Float variant kept for the CUDA path.
std::vector <float> linspace1(double a, double b, std::int32_t N);

}

#endif
