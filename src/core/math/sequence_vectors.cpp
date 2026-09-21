/**
 * @file
 * @brief Sequence wrappers over the math library.
 *
 * The double variants forward to the index-based generators; the float
 * variant computes its own step and guards the single-point case.
 */

#include <vector>
#include <cstdint>
#include "src/core/math/sequence_vectors.h"

#include "src/core/math/sequences.h"

std::vector <double> qftbx::linspace(double a, double b, std::int32_t N) {
    return qftbx::math::linspace(a, b, static_cast<std::size_t>(N > 0 ? N : 0));
}

std::vector<float> qftbx::linspace1(double a, double b, std::int32_t N){
    if (N <= 0){
        return std::vector<float>();
    }
    if (N == 1){
        return std::vector<float>(1, static_cast<float>(a));
    }

    float h = (b - a) / (N-1);
    std::vector<float> vec;
    vec.reserve(static_cast<std::size_t>(N));

    float val = a;

    for (std::int32_t i = 0; i < N; i++){
        vec.push_back(val);
        val+=h;
    }

    return vec;
}

std::vector <double> qftbx::logspace (double a, double b, std::int32_t N){
    return qftbx::math::logspace(a, b, static_cast<std::size_t>(N > 0 ? N : 0));
}
