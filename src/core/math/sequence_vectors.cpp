#include <vector>
#include <cstdint>
#include "src/core/math/sequence_vectors.h"


#include "src/core/math/sequences.h"




//Wrapper over the canonical implementation in src/core/math/ (no
//accumulation drift, exact final endpoint).
std::vector <double> tools::linspace(double a, double b, std::int32_t N) {
    //Returned as computed: this used to copy the result into a second vector
    //for no reason.
    return qftbx::math::linspace(a, b, static_cast<std::size_t>(N > 0 ? N : 0));
}


std::vector<float> tools::linspace1(double a, double b, std::int32_t N){
    //N == 1 divided by zero here. The canonical linspace was fixed for it and
    //this float variant, kept for the CUDA path, was not.
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

    for (std::int32_t i = 0; i < N; i++){ // https://gist.github.com/jmbr/2375233
        vec.push_back(val);
        val+=h;
    }

    return vec;
}


//See tools::linspace.
std::vector <double> tools::logspace (double a, double b, std::int32_t N){
    return qftbx::math::logspace(a, b, static_cast<std::size_t>(N > 0 ? N : 0));
}
