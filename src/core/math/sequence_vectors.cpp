/**
 * @file
 * @brief Sequence wrappers over the math library.
 *
 * Both forward to the index-based generators.
 */

#include <vector>
#include <cstdint>
#include "src/core/math/sequence_vectors.h"

#include "src/core/math/sequences.h"

std::vector <double> qftbx::linspace(double a, double b, std::int32_t N) {
    return qftbx::math::linspace(a, b, static_cast<std::size_t>(N > 0 ? N : 0));
}

std::vector <double> qftbx::logspace (double a, double b, std::int32_t N){
    return qftbx::math::logspace(a, b, static_cast<std::size_t>(N > 0 ? N : 0));
}
