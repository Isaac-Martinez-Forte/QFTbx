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

std::vector <double> linspace(double a, double b, std::int32_t N);
std::vector <double> logspace (double a, double b, std::int32_t N);

std::vector <float> linspace1(double a, double b, std::int32_t N);

}

#endif
