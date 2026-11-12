/**
 * @file
 * @brief A plant family on the card: its upload and the choice of kernel.
 *
 * dispatch() calls back with the form of the plant, whether the sweep walks
 * the border and the width of the index as types, so a launch picks the
 * kernel built for them. flatten() lays the grids end to end, viewOf()
 * points the views at host memory, and upload() copies the model, the grid
 * and the frequencies to the card and points the views there: plant
 * evaluates the ratio of a free form and denominator its denominator.
 */

#ifndef QFTBX_GPU_PLANT_ON_CARD_CUH
#define QFTBX_GPU_PLANT_ON_CARD_CUH

#include <cstdint>
#include <type_traits>
#include <vector>

#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/plant_evaluation.cuh"
#include "src/core/gpu/templates/plant_model.h"

namespace qftbx::gpu {

template <PlantForm Form>
using FormTag = std::integral_constant<PlantForm, Form>;

template <typename Visit>
inline void dispatch(PlantForm form, bool border, bool wide, Visit && visit)
{
    const auto withIndex = [&](auto formTag, auto borderTag) {
        if (wide) {
            visit(formTag, borderTag, std::uint64_t{});
        } else {
            visit(formTag, borderTag, std::uint32_t{});
        }
    };
    const auto withBorder = [&](auto formTag) {
        if (border) {
            withIndex(formTag, std::true_type{});
        } else {
            withIndex(formTag, std::false_type{});
        }
    };
    switch (form) {
    case PlantForm::Polynomial: withBorder(FormTag<PlantForm::Polynomial>{}); break;
    case PlantForm::ZeroPoleGain: withBorder(FormTag<PlantForm::ZeroPoleGain>{}); break;
    case PlantForm::TimeConstantGain: withBorder(FormTag<PlantForm::TimeConstantGain>{}); break;
    case PlantForm::FreeForm: withBorder(FormTag<PlantForm::FreeForm>{}); break;
    }
}

struct FlatGrid
{
    std::vector<double> values;
    std::vector<std::int64_t> offset;
    std::vector<std::int64_t> size;
    std::vector<double> border;
};

inline FlatGrid flatten(const SweepGrid & grid)
{
    FlatGrid flat;
    for (const std::vector<double> & values : grid.grids) {
        flat.offset.push_back(static_cast<std::int64_t>(flat.values.size()));
        flat.size.push_back(static_cast<std::int64_t>(values.size()));
        flat.values.insert(flat.values.end(), values.begin(), values.end());
    }
    for (const std::array<double, 2> & point : grid.border) {
        flat.border.push_back(point[0]);
        flat.border.push_back(point[1]);
    }
    return flat;
}

inline PlantView viewOf(const PlantModel & plant)
{
    PlantView view{};
    view.form = plant.form;
    view.numeratorCount = static_cast<std::int32_t>(plant.numeratorSlot.size());
    view.denominatorCount = static_cast<std::int32_t>(plant.denominatorSlot.size());
    view.numeratorSlot = plant.numeratorSlot.data();
    view.numeratorNominal = plant.numeratorNominal.data();
    view.denominatorSlot = plant.denominatorSlot.data();
    view.denominatorNominal = plant.denominatorNominal.data();
    view.gainSlot = plant.gainSlot;
    view.delaySlot = plant.delaySlot;
    view.gainNominal = plant.gainNominal;
    view.delayNominal = plant.delayNominal;
    view.program = plant.program.data();
    view.programLength = static_cast<std::int32_t>(plant.program.size());
    view.numeratorValueSlot = plant.numeratorValueSlot.data();
    view.denominatorValueSlot = plant.denominatorValueSlot.data();
    view.valueCount = plant.valueCount;
    return view;
}

inline GridView viewOf(const SweepGrid & grid, const FlatGrid & flat)
{
    GridView view{};
    view.digitCount = static_cast<std::int32_t>(grid.grids.size());
    view.values = flat.values.data();
    view.offset = flat.offset.data();
    view.size = flat.size.data();
    view.border = grid.border.empty() ? nullptr : flat.border.data();
    return view;
}

struct ModelOnCard
{
    DeviceBuffer<std::int32_t> numeratorSlot;
    DeviceBuffer<double> numeratorNominal;
    DeviceBuffer<std::int32_t> denominatorSlot;
    DeviceBuffer<double> denominatorNominal;
    DeviceBuffer<ProgramStep> program;
    DeviceBuffer<ProgramStep> denominatorProgram;
    DeviceBuffer<std::int32_t> numeratorValueSlot;
    DeviceBuffer<std::int32_t> denominatorValueSlot;
    DeviceBuffer<double> gridValues;
    DeviceBuffer<std::int64_t> gridOffset;
    DeviceBuffer<std::int64_t> gridSize;
    DeviceBuffer<double> border;
    DeviceBuffer<double> frequencies;

    PlantView plant{};
    PlantView denominator{};
    GridView grid{};
};

inline void upload(ModelOnCard & card, const PlantModel & plant, const SweepGrid & grid, const FlatGrid & flat,
            const std::vector<double> & frequencies, const Stream & stream)
{
    card.numeratorSlot = uploaded(plant.numeratorSlot, stream);
    card.numeratorNominal = uploaded(plant.numeratorNominal, stream);
    card.denominatorSlot = uploaded(plant.denominatorSlot, stream);
    card.denominatorNominal = uploaded(plant.denominatorNominal, stream);
    card.program = uploaded(plant.program, stream);
    card.denominatorProgram = uploaded(plant.denominatorProgram, stream);
    card.numeratorValueSlot = uploaded(plant.numeratorValueSlot, stream);
    card.denominatorValueSlot = uploaded(plant.denominatorValueSlot, stream);
    card.gridValues = uploaded(flat.values, stream);
    card.gridOffset = uploaded(flat.offset, stream);
    card.gridSize = uploaded(flat.size, stream);
    card.border = uploaded(flat.border, stream);
    card.frequencies = uploaded(frequencies, stream);

    card.plant = viewOf(plant);
    card.plant.numeratorSlot = card.numeratorSlot.data();
    card.plant.numeratorNominal = card.numeratorNominal.data();
    card.plant.denominatorSlot = card.denominatorSlot.data();
    card.plant.denominatorNominal = card.denominatorNominal.data();
    card.plant.program = card.program.data();
    card.plant.numeratorValueSlot = card.numeratorValueSlot.data();
    card.plant.denominatorValueSlot = card.denominatorValueSlot.data();
    card.denominator = card.plant;
    card.denominator.program = card.denominatorProgram.data();
    card.denominator.programLength = static_cast<std::int32_t>(plant.denominatorProgram.size());

    card.grid = viewOf(grid, flat);
    card.grid.values = card.gridValues.data();
    card.grid.offset = card.gridOffset.data();
    card.grid.size = card.gridSize.data();
    card.grid.border = grid.border.empty() ? nullptr : card.border.data();
}

}

#endif
