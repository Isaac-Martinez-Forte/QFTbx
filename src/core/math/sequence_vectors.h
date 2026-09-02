#ifndef QFTBX_MATH_SEQUENCE_VECTORS_H
#define QFTBX_MATH_SEQUENCE_VECTORS_H

#include <cstdint>
#include <vector>

#include <QString>
#include <QVector>

//Transitional re-exports: these types moved to their own homes; consumers
//will include them directly as each module is migrated.
#include "src/core/specifications/specification_record.h"
#include "src/core/text_tokens.h"
#include "src/core/loopshaping/loop_shaping_types.h"

/**
 * @namespace tools
 * @brief QVector flavours of the numeric sequences, for the consumers that
 * speak Qt containers. They wrap the canonical std implementations in
 * src/core/math/sequences.h.
 */
namespace tools{

//Wrappers over qftbx::math (src/core/math/sequences.h).
QVector <double> linspace(double a, double b, std::int32_t N);
QVector <double> logspace (double a, double b, std::int32_t N);

//Float variant kept verbatim for the CUDA path (deferred).
std::vector <float> linspace1(double a, double b, std::int32_t N);

}

#endif // QFTBX_MATH_SEQUENCE_VECTORS_H
