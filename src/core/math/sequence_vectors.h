#ifndef QFTBX_MATH_SEQUENCE_VECTORS_H
#define QFTBX_MATH_SEQUENCE_VECTORS_H

#include <cstdint>
#include <vector>

//This header used to re-export specification_record.h, text_tokens.h and
//loop_shaping_types.h "until each module is migrated"; the eight files that
//leaned on that include what they use now.

namespace tools{

//Wrappers over qftbx::math (src/core/math/sequences.h).
std::vector <double> linspace(double a, double b, std::int32_t N);
std::vector <double> logspace (double a, double b, std::int32_t N);

//Float variant kept verbatim for the CUDA path (deferred).
std::vector <float> linspace1(double a, double b, std::int32_t N);

}

#endif // QFTBX_MATH_SEQUENCE_VECTORS_H
