/**
 * @file
 * @brief The plant family as the card evaluates it.
 *
 * PlantModel says how to evaluate one plant from the digits of a
 * combination: its form, where each coefficient comes from (a digit, or -1
 * for its nominal value), and for a free-form plant the program of its
 * ratio and the slot of each parameter in the program's values, slot 0
 * being s. SweepGrid is the combinations: the grids, the first changing
 * fastest, or the points of the border. Plain C++, built by the glue from
 * the plant and the sweep plan, and read by the kernels. The limits are
 * what a thread keeps in its registers.
 */

#ifndef QFTBX_GPU_PLANT_MODEL_H
#define QFTBX_GPU_PLANT_MODEL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "src/core/math/expression_program.h"

namespace qftbx::gpu {

enum class PlantForm : std::int32_t { Polynomial, ZeroPoleGain, TimeConstantGain, FreeForm };

inline constexpr int kMaxDigits = 16;
inline constexpr int kMaxValues = 32;
inline constexpr int kMaxDepth = 32;

struct PlantModel
{
    PlantForm form = PlantForm::Polynomial;
    std::vector<std::int32_t> numeratorSlot;
    std::vector<double> numeratorNominal;
    std::vector<std::int32_t> denominatorSlot;
    std::vector<double> denominatorNominal;
    std::int32_t gainSlot = -1;
    std::int32_t delaySlot = -1;
    double gainNominal = 0.0;
    double delayNominal = 0.0;

    std::vector<ProgramStep> program;
    std::int32_t programDepth = 0;
    std::vector<std::int32_t> numeratorValueSlot;
    std::vector<std::int32_t> denominatorValueSlot;
    std::int32_t valueCount = 0;
};

struct SweepGrid
{
    std::vector<std::vector<double>> grids;
    std::size_t combinations = 0;
    std::vector<std::array<double, 2>> border;
};

}

#endif
