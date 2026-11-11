/**
 * @file
 * @brief The sweep kernel and its run.
 *
 * The launch grid is two-dimensional: a row of blocks per frequency of the
 * batch, so a thread finds its frequency without dividing, and in a row
 * consecutive threads on consecutive combinations, so the writes
 * coalesce. There is one kernel per form of the plant, sweep of the border
 * and width of the index, 32 bits while the combinations stay under 2^31,
 * so that the stride cannot wrap, and dispatch() picks it when launching.
 * A warp that met a plant that is not finite marks its frequency with one
 * vote and one write. The clouds come back with a plain copy
 * (DeviceComplex).
 */

#include "src/core/gpu/templates/plant_sweep.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/launch.cuh"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/device_clouds.cuh"
#include "src/core/gpu/templates/plant_evaluation.cuh"

namespace qftbx::gpu {

namespace {

template <PlantForm Form, bool Border, typename Index>
__global__ void sweep(PlantView plant, GridView grid, const double * frequencies, std::size_t first,
                      Index combinations, DeviceComplex * clouds, int * nonFinite)
{
    const std::size_t frequency = blockIdx.y;
    const double w = frequencies[first + frequency];
    DeviceComplex * cloud = clouds + frequency * static_cast<std::size_t>(combinations);
    const Index stride = static_cast<Index>(gridDim.x) * static_cast<Index>(blockDim.x);

    bool notFinite = false;
    for (Index i = static_cast<Index>(blockIdx.x) * static_cast<Index>(blockDim.x) + static_cast<Index>(threadIdx.x);
         i < combinations; i += stride) {
        double digit[kMaxDigits];
        digitsOf<Border, Index>(grid, i, digit);
        const DeviceComplex value = plantAt<Form>(plant, digit, w);
        cloud[i] = value;
        notFinite |= !(isfinite(value.real()) && isfinite(value.imag()));
    }
    if (__any_sync(0xffffffffu, notFinite) && threadIdx.x % warpSize == 0) {
        nonFinite[first + frequency] = 1;
    }
}

template <PlantForm Form>
using FormTag = std::integral_constant<PlantForm, Form>;

template <typename Visit>
void dispatch(PlantForm form, bool border, bool wide, Visit && visit)
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

FlatGrid flatten(const SweepGrid & grid)
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

PlantView viewOf(const PlantModel & plant)
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

GridView viewOf(const SweepGrid & grid, const FlatGrid & flat)
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
    DeviceBuffer<std::int32_t> numeratorValueSlot;
    DeviceBuffer<std::int32_t> denominatorValueSlot;
    DeviceBuffer<double> gridValues;
    DeviceBuffer<std::int64_t> gridOffset;
    DeviceBuffer<std::int64_t> gridSize;
    DeviceBuffer<double> border;
    DeviceBuffer<double> frequencies;

    PlantView plant{};
    GridView grid{};
};

void upload(ModelOnCard & card, const PlantModel & plant, const SweepGrid & grid, const FlatGrid & flat,
            const std::vector<double> & frequencies, const Stream & stream)
{
    card.numeratorSlot = uploaded(plant.numeratorSlot, stream);
    card.numeratorNominal = uploaded(plant.numeratorNominal, stream);
    card.denominatorSlot = uploaded(plant.denominatorSlot, stream);
    card.denominatorNominal = uploaded(plant.denominatorNominal, stream);
    card.program = uploaded(plant.program, stream);
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

    card.grid = viewOf(grid, flat);
    card.grid.values = card.gridValues.data();
    card.grid.offset = card.gridOffset.data();
    card.grid.size = card.gridSize.data();
    card.grid.border = grid.border.empty() ? nullptr : card.border.data();
}

double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

}

SweepOnCard sweepOnCard(const PlantModel & plant, const SweepGrid & grid, const std::vector<double> & frequencies,
                        const CancellationToken * cancellation)
{
    SweepOnCard result;
    const std::size_t frequencyCount = frequencies.size();
    const std::size_t plants = grid.combinations;
    result.clouds.assign(frequencyCount, {});
    result.nonFinite.assign(frequencyCount, 0);
    if (frequencyCount == 0 || plants == 0) {
        return result;
    }

    useProbedDevice();
    auto stream = std::make_shared<Stream>();
    const FlatGrid flat = flatten(grid);
    ModelOnCard card;
    upload(card, plant, grid, flat, frequencies, *stream);
    DeviceBuffer<int> nonFinite(frequencyCount, *stream);
    nonFinite.fill(0);

    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    check(cudaMemGetInfo(&freeBytes, &totalBytes), "cudaMemGetInfo");
    const std::size_t perFrequency = plants * sizeof(DeviceComplex);
    if (perFrequency > freeBytes / 2) {
        throw CudaError("the card's free memory cannot hold the plants of one frequency", 0);
    }
    const std::size_t batch = std::min<std::size_t>({frequencyCount, (freeBytes / 2) / perFrequency, 65535});

    DeviceBuffer<DeviceComplex> clouds(batch * plants, *stream);
    PinnedBuffer<DeviceComplex> staging(batch * plants);
    const bool wide = plants >= (std::size_t{1} << 31);

    for (std::size_t first = 0; first < frequencyCount; first += batch) {
        const std::size_t count = std::min(batch, frequencyCount - first);

        const auto computing = std::chrono::steady_clock::now();
        dispatch(plant.form, !grid.border.empty(), wide, [&](auto formTag, auto borderTag, auto index) {
            using Index = decltype(index);
            const auto kernel = sweep<decltype(formTag)::value, decltype(borderTag)::value, Index>;
            const int block = blockSizeFor(kernel);
            const dim3 blocks(blocksFor(plants, block), static_cast<unsigned int>(count));
            kernel<<<blocks, block, 0, stream->get()>>>(card.plant, card.grid, card.frequencies.data(), first,
                                                        static_cast<Index>(plants), clouds.data(), nonFinite.data());
        });
        checkLaunch("the sweep kernel");
        if (!stream->finish(cancellation, nullptr)) {
            result.cancelled = true;
            return result;
        }
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        check(cudaMemcpyAsync(staging.data(), clouds.data(), count * perFrequency, cudaMemcpyDeviceToHost,
                              stream->get()), "cudaMemcpyAsync of the clouds");
        stream->synchronize();
        for (std::size_t u = 0; u < count; ++u) {
            std::vector<std::complex<double>> & cloud = result.clouds[first + u];
            cloud.resize(plants);
            std::memcpy(static_cast<void *>(cloud.data()), staging.data() + u * plants, perFrequency);
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }

    std::vector<int> flags(frequencyCount, 0);
    nonFinite.download(flags.data());
    stream->synchronize();
    for (std::size_t u = 0; u < frequencyCount; ++u) {
        result.nonFinite[u] = flags[u] != 0 ? 1 : 0;
    }

    if (batch >= frequencyCount) {
        result.resident = std::make_shared<const DeviceClouds>(stream, std::move(clouds), frequencyCount, plants);
    }
    return result;
}

std::complex<double> plantOnProcessor(const PlantModel & plant, const std::vector<double> & digits, double w)
{
    DeviceComplex value;
    dispatch(plant.form, false, false, [&](auto formTag, auto, auto) {
        value = plantAt<decltype(formTag)::value>(viewOf(plant), digits.data(), w);
    });
    return {value.real(), value.imag()};
}

std::vector<double> digitsOnProcessor(const SweepGrid & grid, std::size_t combination)
{
    const FlatGrid flat = flatten(grid);
    std::vector<double> digits(std::max<std::size_t>(grid.grids.size(), 2));
    if (grid.border.empty()) {
        digitsOf<false, std::size_t>(viewOf(grid, flat), combination, digits.data());
    } else {
        digitsOf<true, std::size_t>(viewOf(grid, flat), combination, digits.data());
    }
    digits.resize(grid.grids.size());
    return digits;
}

}
