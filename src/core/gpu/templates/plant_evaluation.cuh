/**
 * @file
 * @brief One plant of the family at one frequency, as a thread evaluates
 * it, on the card or on the processor.
 *
 * The views point at the arrays of a PlantModel and a SweepGrid wherever
 * they live. digitsOf() gives the digits of a combination, the first grid
 * changing fastest, or a point of the border. plantAt() performs the
 * operations of the plant's valueAt() in the same order: Horner for the
 * polynomial form, the products of (s + root) and (s/c + 1) for the other
 * two closed forms, and the postfix program for the free form, with the
 * parameters in their slots (bindValues), then the gain and the delay. The arithmetic is libcu++'s, so the values agree
 * with the processor's to rounding, not to the bit.
 *
 * The form of the plant, whether the sweep walks the border and the width
 * of the combination's index are template parameters, chosen when the
 * kernel is launched: a thread never branches on them, and the digits are
 * taken with 32-bit divisions whenever the combinations fit. What still
 * branches inside, a coefficient's source and the program's operation,
 * goes the same way in every thread of a warp, since all evaluate the same
 * plant.
 */

#ifndef QFTBX_GPU_PLANT_EVALUATION_CUH
#define QFTBX_GPU_PLANT_EVALUATION_CUH

#include <cmath>
#include <cstddef>
#include <cstdint>

#include <cuda/std/complex>

#include "src/core/gpu/common/device_complex.cuh"
#include "src/core/gpu/common/host_device.h"
#include "src/core/gpu/templates/plant_model.h"

namespace qftbx::gpu {

struct PlantView
{
    PlantForm form;
    std::int32_t numeratorCount;
    std::int32_t denominatorCount;
    const std::int32_t * numeratorSlot;
    const double * numeratorNominal;
    const std::int32_t * denominatorSlot;
    const double * denominatorNominal;
    std::int32_t gainSlot;
    std::int32_t delaySlot;
    double gainNominal;
    double delayNominal;
    const ProgramStep * program;
    std::int32_t programLength;
    const std::int32_t * numeratorValueSlot;
    const std::int32_t * denominatorValueSlot;
    std::int32_t valueCount;
};

struct GridView
{
    std::int32_t digitCount;
    const double * values;
    const std::int64_t * offset;
    const std::int64_t * size;
    const double * border;
};

template <bool Border, typename Index>
QFTBX_HD inline void digitsOf(const GridView & grid, Index combination, double * digit)
{
    if constexpr (Border) {
        digit[0] = grid.border[2 * static_cast<std::size_t>(combination)];
        digit[1] = grid.border[2 * static_cast<std::size_t>(combination) + 1];
    } else {
        Index rest = combination;
        for (std::int32_t j = 0; j < grid.digitCount; ++j) {
            const Index size = static_cast<Index>(grid.size[j]);
            digit[j] = grid.values[grid.offset[j] + static_cast<std::int64_t>(rest % size)];
            rest /= size;
        }
    }
}

QFTBX_HD inline double coefficient(std::int32_t slot, double nominal, const double * digit)
{
    return slot >= 0 ? digit[slot] : nominal;
}

QFTBX_HD inline DeviceComplex wholePower(DeviceComplex base, int exponent)
{
    unsigned count = exponent < 0 ? 0u - static_cast<unsigned>(exponent) : static_cast<unsigned>(exponent);
    DeviceComplex power = count % 2 != 0 ? base : DeviceComplex(1.0, 0.0);
    while ((count >>= 1) != 0) {
        base = base * base;
        if (count % 2 != 0) {
            power = power * base;
        }
    }
    return exponent < 0 ? DeviceComplex(1.0, 0.0) / power : power;
}

QFTBX_HD inline DeviceComplex runProgram(const ProgramStep * steps, std::int32_t length, const DeviceComplex * values)
{
    DeviceComplex stack[kMaxDepth];
    std::int32_t top = 0;
    for (std::int32_t k = 0; k < length; ++k) {
        const ProgramStep & step = steps[k];
        switch (step.op) {
        case ProgramOp::Constant: stack[top++] = DeviceComplex(step.constant, 0.0); break;
        case ProgramOp::Variable: stack[top++] = values[step.operand]; break;
        case ProgramOp::Add: --top; stack[top - 1] = stack[top - 1] + stack[top]; break;
        case ProgramOp::Subtract: --top; stack[top - 1] = stack[top - 1] - stack[top]; break;
        case ProgramOp::Multiply: --top; stack[top - 1] = stack[top - 1] * stack[top]; break;
        case ProgramOp::Divide: --top; stack[top - 1] = stack[top - 1] / stack[top]; break;
        case ProgramOp::Power: --top; stack[top - 1] = cuda::std::pow(stack[top - 1], stack[top]); break;
        case ProgramOp::WholePower: stack[top - 1] = wholePower(stack[top - 1], step.operand); break;
        case ProgramOp::Sin: stack[top - 1] = cuda::std::sin(stack[top - 1]); break;
        case ProgramOp::Cos: stack[top - 1] = cuda::std::cos(stack[top - 1]); break;
        case ProgramOp::Tan: stack[top - 1] = cuda::std::tan(stack[top - 1]); break;
        case ProgramOp::Atan: stack[top - 1] = cuda::std::atan(stack[top - 1]); break;
        case ProgramOp::Sinh: stack[top - 1] = cuda::std::sinh(stack[top - 1]); break;
        case ProgramOp::Cosh: stack[top - 1] = cuda::std::cosh(stack[top - 1]); break;
        case ProgramOp::Tanh: stack[top - 1] = cuda::std::tanh(stack[top - 1]); break;
        case ProgramOp::Asin: stack[top - 1] = cuda::std::asin(stack[top - 1]); break;
        case ProgramOp::Acos: stack[top - 1] = cuda::std::acos(stack[top - 1]); break;
        case ProgramOp::Exp: stack[top - 1] = cuda::std::exp(stack[top - 1]); break;
        case ProgramOp::Abs: stack[top - 1] = DeviceComplex(cuda::std::abs(stack[top - 1]), 0.0); break;
        case ProgramOp::Ln: stack[top - 1] = cuda::std::log(stack[top - 1]); break;
        case ProgramOp::Lg: stack[top - 1] = cuda::std::log10(stack[top - 1]); break;
        case ProgramOp::Log2: stack[top - 1] = cuda::std::log(stack[top - 1]) / log(2.0); break;
        case ProgramOp::Sqrt: stack[top - 1] = cuda::std::sqrt(stack[top - 1]); break;
        }
    }
    return stack[0];
}

QFTBX_HD inline void bindValues(const PlantView & plant, const double * digit, DeviceComplex * values)
{
    for (std::int32_t c = 0; c < plant.numeratorCount; ++c) {
        values[plant.numeratorValueSlot[c]] =
                DeviceComplex(coefficient(plant.numeratorSlot[c], plant.numeratorNominal[c], digit), 0.0);
    }
    for (std::int32_t c = 0; c < plant.denominatorCount; ++c) {
        values[plant.denominatorValueSlot[c]] =
                DeviceComplex(coefficient(plant.denominatorSlot[c], plant.denominatorNominal[c], digit), 0.0);
    }
}

template <PlantForm Form>
QFTBX_HD inline void closedFormParts(const PlantView & plant, const double * digit, DeviceComplex s,
                                     DeviceComplex & numerator, DeviceComplex & denominator)
{
    numerator = DeviceComplex(1.0, 0.0);
    denominator = DeviceComplex(1.0, 0.0);
    if constexpr (Form == PlantForm::Polynomial) {
        if (plant.numeratorCount > 0) {
            numerator = DeviceComplex(coefficient(plant.numeratorSlot[0], plant.numeratorNominal[0], digit), 0.0);
            for (std::int32_t c = 1; c < plant.numeratorCount; ++c) {
                numerator = numerator * s + coefficient(plant.numeratorSlot[c], plant.numeratorNominal[c], digit);
            }
        }
        if (plant.denominatorCount > 0) {
            denominator = DeviceComplex(coefficient(plant.denominatorSlot[0], plant.denominatorNominal[0], digit), 0.0);
            for (std::int32_t c = 1; c < plant.denominatorCount; ++c) {
                denominator = denominator * s + coefficient(plant.denominatorSlot[c], plant.denominatorNominal[c], digit);
            }
        }
    } else if constexpr (Form == PlantForm::ZeroPoleGain) {
        for (std::int32_t c = 0; c < plant.numeratorCount; ++c) {
            numerator *= s + coefficient(plant.numeratorSlot[c], plant.numeratorNominal[c], digit);
        }
        for (std::int32_t c = 0; c < plant.denominatorCount; ++c) {
            denominator *= s + coefficient(plant.denominatorSlot[c], plant.denominatorNominal[c], digit);
        }
    } else {
        for (std::int32_t c = 0; c < plant.numeratorCount; ++c) {
            numerator *= s / coefficient(plant.numeratorSlot[c], plant.numeratorNominal[c], digit) + 1.0;
        }
        for (std::int32_t c = 0; c < plant.denominatorCount; ++c) {
            denominator *= s / coefficient(plant.denominatorSlot[c], plant.denominatorNominal[c], digit) + 1.0;
        }
    }
}

template <PlantForm Form>
QFTBX_HD inline DeviceComplex plantAt(const PlantView & plant, const double * digit, double w)
{
    const DeviceComplex s(0.0, w);
    const double gain = coefficient(plant.gainSlot, plant.gainNominal, digit);
    const double delay = coefficient(plant.delaySlot, plant.delayNominal, digit);

    if constexpr (Form == PlantForm::FreeForm) {
        DeviceComplex values[kMaxValues];
        bindValues(plant, digit, values);
        values[0] = s;
        const DeviceComplex ratio = runProgram(plant.program, plant.programLength, values);
        return gain * ratio * cuda::std::exp(-s * delay);
    } else {
        DeviceComplex numerator;
        DeviceComplex denominator;
        closedFormParts<Form>(plant, digit, s, numerator, denominator);
        return gain * numerator / denominator * cuda::std::exp(-s * delay);
    }
}

}

#endif
