/**
 * @file
 * @brief The kernels of the sheets, the batches that launch them, and the
 * run that brings the sheets back.
 *
 * A batch of frequencies (SheetBatch) goes up with its templates, and on
 * the card:
 * - prepareSamples() takes P0/P, its radius and its single precision copy,
 *   one thread per sample, and measureScale() the largest radius of each
 *   frequency, which says whether single precision holds;
 * - for a contour with the guard, buildSegments() splits it into loops where
 *   the starting point repeats, as SingularLocus does, one warp per
 *   frequency that looks for the repeat 32 samples at a time;
 * - for a cloud with the guard, spacings() gives each sample the distance
 *   to its nearest neighbour, one thread per sample;
 * - the cell kernels, one block per run of phases of a magnitude row of a
 *   frequency, so that |L| and the template are the block's: the template
 *   goes through shared memory in tiles that every thread reads at once,
 *   and a warp evaluates a sample or a segment in double precision only
 *   when one of its threads needs it, all its threads together.
 *
 * What a frequency needs is decided when launching: one kernel per set of
 * extremes and per guard, and the frequencies that need the same share a
 * launch, their indices in the third dimension of the grid. Inside, every
 * branch goes the same way for a whole block or a whole warp.
 * sheetsOnCard() brings the sheets back through pinned memory into the
 * processor's layout; boundariesOnCard() reads them where they are.
 */

#include "src/core/gpu/boundaries/sheet_sweep.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>

#include <cub/block/block_reduce.cuh>
#include <cuda_runtime.h>

#include "src/core/gpu/boundaries/sheet_batch.cuh"
#include "src/core/gpu/boundaries/sheet_bounds.cuh"
#include "src/core/gpu/common/cancel_flag.cuh"
#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/cuda_error.h"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/launch.cuh"
#include "src/core/gpu/common/stream.cuh"

namespace qftbx::gpu {

namespace {

constexpr std::int32_t kCellBlock = 128;
constexpr std::int32_t kScaleBlock = 256;
constexpr unsigned int kFullWarp = 0xffffffffu;

struct Larger
{
    __device__ double operator()(double a, double b) const { return a >= b ? a : b; }
};

__global__ void prepareSamples(BatchView v)
{
    const std::int32_t u = static_cast<std::int32_t>(blockIdx.y);
    const std::int32_t i = static_cast<std::int32_t>(blockIdx.x * blockDim.x + threadIdx.x);
    if (i >= v.count[u]) {
        return;
    }
    const std::size_t index = v.offset[u] + static_cast<std::size_t>(i);
    prepareSample(v.nominal[u], v.plants[index], v.q[index], v.radius[index], v.single[index]);
}

__global__ void __launch_bounds__(kScaleBlock) measureScale(BatchView v)
{
    using Reduce = cub::BlockReduce<double, kScaleBlock>;
    __shared__ typename Reduce::TempStorage storage;

    const std::int32_t u = static_cast<std::int32_t>(blockIdx.x);
    const std::size_t first = v.offset[u];
    const std::int32_t count = v.count[u];
    double largest = 0.0;
    for (std::int32_t i = static_cast<std::int32_t>(threadIdx.x); i < count; i += kScaleBlock) {
        const double radius = v.radius[first + static_cast<std::size_t>(i)];
        largest = radius <= largest ? largest : (radius == radius ? radius : kInfinity);
    }
    const double total = Reduce(storage).Reduce(largest, Larger{});
    if (threadIdx.x == 0) {
        v.largest[u] = total;
        v.scale[u] = total <= 1e38 ? static_cast<float>(total) * kRadiusUp : kInfinityF;
    }
}

__global__ void buildSegments(BatchView v)
{
    const std::int32_t u = static_cast<std::int32_t>(blockIdx.x);
    const std::int32_t lane = static_cast<std::int32_t>(threadIdx.x);
    const std::size_t first = v.offset[u];
    const std::int32_t n = v.count[u];
    const DeviceComplex * q = v.q + first;

    std::int32_t start = 0;
    while (start < n) {
        const DeviceComplex head = q[start];
        std::int32_t end = n;
        for (std::int32_t base = start + 1; base < n; base += 32) {
            const std::int32_t j = base + lane;
            const unsigned int found = __ballot_sync(kFullWarp, j < n && q[j] == head);
            if (found != 0) {
                end = base + __ffs(static_cast<int>(found)) - 1;
                break;
            }
        }
        const std::int32_t last = end < n ? end : n - 1;
        for (std::int32_t j = start + lane; j <= last; j += 32) {
            const DeviceComplex a = q[j];
            const bool inLoop = j < end;
            const DeviceComplex b = inLoop ? (j + 1 < end ? q[j + 1] : head) : a;
            prepareSegment(a, b, inLoop && j + 1 == end, v.segment[first + static_cast<std::size_t>(j)],
                           v.segmentExact[first + static_cast<std::size_t>(j)]);
        }
        start = end + 1;
    }
}

__global__ void __launch_bounds__(kCellBlock) spacings(BatchView v)
{
    __shared__ SampleFloats tile[kCellBlock];

    const std::int32_t u = static_cast<std::int32_t>(blockIdx.y);
    const std::int32_t count = v.count[u];
    if (static_cast<std::int32_t>(blockIdx.x) * kCellBlock >= count || cancelRequested(v.cancel)) {
        return;
    }
    const std::size_t first = v.offset[u];
    const std::int32_t i = static_cast<std::int32_t>(blockIdx.x) * kCellBlock + static_cast<std::int32_t>(threadIdx.x);
    const std::int32_t self = i < count ? i : count - 1;
    const SampleFloats me = v.single[first + static_cast<std::size_t>(self)];
    const DeviceComplex w = -v.q[first + static_cast<std::size_t>(self)];
    const CellView cell{0, me.x, me.y, me.radius, 0.0f, 0.0f, 0.0f, 0.0f};
    const bool everything = !(2.0f * v.scale[u] <= kLargest);
    const Shrink shrink = shrinkOf(2.0 * v.largest[u]);

    Bounds bounds;
    if (!everything) {
        for (std::int32_t start = 0; start < count; start += kCellBlock) {
            const std::int32_t n = ::min(kCellBlock, count - start);
            __syncthreads();
            if (static_cast<std::int32_t>(threadIdx.x) < n) {
                tile[threadIdx.x] = v.single[first + static_cast<std::size_t>(start) + threadIdx.x];
            }
            __syncthreads();
            for (std::int32_t j = 0; j < n; ++j) {
                Bounds other = bounds;
                boundSample<false>(other, tile[j], sampleBase<false>(tile[j], cell), cell, true, false, false);
                bounds.nearHi = start + j == self ? bounds.nearHi : other.nearHi;
            }
        }
    }

    double nearest2 = kInfinity;
    for (std::int32_t start = 0; start < count; start += kCellBlock) {
        const std::int32_t n = ::min(kCellBlock, count - start);
        __syncthreads();
        if (static_cast<std::int32_t>(threadIdx.x) < n) {
            tile[threadIdx.x] = v.single[first + static_cast<std::size_t>(start) + threadIdx.x];
        }
        __syncthreads();
        for (std::int32_t j = 0; j < n; ++j) {
            const Flags f = flagSample<false>(bounds, tile[j], sampleBase<false>(tile[j], cell), cell, everything,
                                              true, false, false);
            const bool candidate = (start + j != self) & f.near;
            if (__any_sync(kFullWarp, candidate)) {
                const double d2 = distance2(v.q[first + static_cast<std::size_t>(start + j)], w, shrink);
                nearest2 = candidate && d2 < nearest2 ? d2 : nearest2;
            }
        }
    }
    if (i < count) {
        const double nearest = ::sqrt(nearest2) / shrink.factor;
        v.spacing[first + static_cast<std::size_t>(i)] = ::isfinite(nearest) ? nearest : 0.0;
    }
}

struct CellPlace
{
    std::int32_t u;
    std::int32_t row;
    std::int32_t column;
    bool active;
    DeviceComplex L;
    CellView view;
    std::size_t first;
    std::int32_t count;
    bool everything;
    bool farField;
    Shrink shrink;
};

__device__ inline CellPlace placeOf(const BatchView & v, const std::int32_t * group)
{
    CellPlace place;
    place.u = group[blockIdx.z];
    place.row = static_cast<std::int32_t>(blockIdx.y);
    place.column = static_cast<std::int32_t>(blockIdx.x) * kCellBlock + static_cast<std::int32_t>(threadIdx.x);
    place.active = place.column < v.phaseCount;
    const std::int32_t c = place.active ? place.column : v.phaseCount - 1;
    const double r = v.rowMagnitude[place.row];
    place.L = DeviceComplex(r * v.cosine[c], r * v.sine[c]);
    place.view = cellView(place.L, r, v.cosine[c], v.sine[c]);
    place.first = v.offset[place.u];
    place.count = v.count[place.u];
    const float scale = v.scale[place.u];
    place.everything = !(scale + place.view.zRadius <= kLargest);
    place.farField = farFieldOf(place.view.zRadius, scale);
    place.shrink = shrinkOf(v.largest[place.u] + r);
    return place;
}

__device__ inline void writeCell(const BatchView & v, const CellPlace & place, const Extremes & x, Guard guard,
                                 double halfSpacing)
{
    const std::int64_t * offsets = v.sheetOffset + 5 * place.u;
    const SheetsUsed used{offsets[0] >= 0, offsets[1] >= 0, offsets[2] >= 0, offsets[3] >= 0, offsets[4] >= 0};
    CellValues values;
    if (place.count > 0) {
        const double loop = ::hypot(place.L.real(), place.L.imag());
        values = decibels(magnitudesOf(x, loop, v.nominalMagnitude[place.u], guard, halfSpacing, place.shrink), used);
    }
    const std::int64_t cell = static_cast<std::int64_t>(place.row) * v.phaseCount + place.column;
    if (used.stabilityNoise) {
        v.sheets[offsets[0] + cell] = values.stabilityNoise;
    }
    if (used.tracking) {
        v.sheets[offsets[1] + cell] = values.tracking;
    }
    if (used.output) {
        v.sheets[offsets[2] + cell] = values.output;
    }
    if (used.input) {
        v.sheets[offsets[3] + cell] = values.input;
    }
    if (used.effort) {
        v.sheets[offsets[4] + cell] = values.effort;
    }
}

template <bool Near, bool Far, bool Ratio, bool Indexed, bool FarField>
__device__ inline Extremes samplePasses(const BatchView & v, const CellPlace & place, SampleFloats * tile,
                                        float * tileBase)
{
    Bounds bounds;
    if (!place.everything) {
        for (std::int32_t start = 0; start < place.count; start += kCellBlock) {
            const std::int32_t n = ::min(kCellBlock, place.count - start);
            __syncthreads();
            if (static_cast<std::int32_t>(threadIdx.x) < n) {
                const SampleFloats sample = v.single[place.first + static_cast<std::size_t>(start) + threadIdx.x];
                tile[threadIdx.x] = sample;
                tileBase[threadIdx.x] = sampleBase<FarField>(sample, place.view);
            }
            __syncthreads();
#pragma unroll 4
            for (std::int32_t i = 0; i < n; ++i) {
                boundSample<FarField>(bounds, tile[i], tileBase[i], place.view, Near, Far, Ratio);
            }
        }
    }

    Extremes x;
    for (std::int32_t start = 0; start < place.count; start += kCellBlock) {
        const std::int32_t n = ::min(kCellBlock, place.count - start);
        __syncthreads();
        if (static_cast<std::int32_t>(threadIdx.x) < n) {
            const SampleFloats sample = v.single[place.first + static_cast<std::size_t>(start) + threadIdx.x];
            tile[threadIdx.x] = sample;
            tileBase[threadIdx.x] = sampleBase<FarField>(sample, place.view);
        }
        __syncthreads();
        for (std::int32_t i = 0; i < n; ++i) {
            const Flags f = flagSample<FarField>(bounds, tile[i], tileBase[i], place.view, place.everything, Near, Far,
                                                 Ratio);
            if (__any_sync(kFullWarp, f.near | f.far | f.ratio)) {
                const std::size_t index = place.first + static_cast<std::size_t>(start + i);
                keepSample<Indexed>(x, f, v.q[index], place.L, v.radius[index], start + i, place.shrink);
            }
        }
    }
    return x;
}

template <bool Near, bool Far, bool Ratio, bool Indexed>
__global__ void __launch_bounds__(kCellBlock) pointCells(BatchView v, const std::int32_t * group)
{
    __shared__ SampleFloats tile[kCellBlock];
    __shared__ float tileBase[kCellBlock];

    if (cancelRequested(v.cancel)) {
        return;
    }
    const CellPlace place = placeOf(v, group);
    const Extremes x = place.farField ? samplePasses<Near, Far, Ratio, Indexed, true>(v, place, tile, tileBase)
                                      : samplePasses<Near, Far, Ratio, Indexed, false>(v, place, tile, tileBase);
    if (place.active) {
        const double halfSpacing = Indexed && x.nearestIndex >= 0
                ? 0.5 * v.spacing[place.first + static_cast<std::size_t>(x.nearestIndex)] : 0.0;
        writeCell(v, place, x, Indexed ? Guard::Cloud : Guard::None, halfSpacing);
    }
}

template <bool Far, bool Ratio, bool FarField>
__device__ inline Extremes segmentPasses(const BatchView & v, const CellPlace & place, SegmentFloats * tile,
                                         SegmentBases * tileBase)
{
    Bounds bounds;
    if (!place.everything) {
        for (std::int32_t start = 0; start < place.count; start += kCellBlock) {
            const std::int32_t n = ::min(kCellBlock, place.count - start);
            __syncthreads();
            if (static_cast<std::int32_t>(threadIdx.x) < n) {
                const SegmentFloats segment = v.segment[place.first + static_cast<std::size_t>(start) + threadIdx.x];
                tile[threadIdx.x] = segment;
                tileBase[threadIdx.x] = segmentBases<FarField>(segment, place.view);
            }
            __syncthreads();
#pragma unroll 2
            for (std::int32_t i = 0; i < n; ++i) {
                boundSegment<FarField>(bounds, tile[i], tileBase[i], place.view, Far, Ratio);
            }
        }
    }

    const DeviceComplex z = -place.L;
    Extremes x;
    std::int32_t winding = 0;
    for (std::int32_t start = 0; start < place.count; start += kCellBlock) {
        const std::int32_t n = ::min(kCellBlock, place.count - start);
        __syncthreads();
        if (static_cast<std::int32_t>(threadIdx.x) < n) {
            const SegmentFloats segment = v.segment[place.first + static_cast<std::size_t>(start) + threadIdx.x];
            tile[threadIdx.x] = segment;
            tileBase[threadIdx.x] = segmentBases<FarField>(segment, place.view);
        }
        __syncthreads();
        for (std::int32_t i = 0; i < n; ++i) {
            const Flags f = flagSegment<FarField>(bounds, tile[i], tileBase[i], place.view, place.everything, Far, Ratio);
            const std::size_t index = place.first + static_cast<std::size_t>(start + i);
            if (__any_sync(kFullWarp, f.near | f.far | f.ratio)) {
                keepSegment<Far>(x, f, z, v.q[index], v.segmentExact[index]);
            }
            if constexpr (!FarField) {
                std::int32_t step = f.step;
                if (__any_sync(kFullWarp, f.doubtful)) {
                    const std::int32_t exact = crossingExact(z, v.q[index], v.segmentExact[index].b);
                    step = f.doubtful ? exact : step;
                }
                winding += step;
                if (tile[i].closes != 0) {
                    x.inside = x.inside || winding != 0;
                    winding = 0;
                }
            }
        }
    }
    return x;
}

template <bool Far, bool Ratio>
__global__ void __launch_bounds__(kCellBlock) segmentCells(BatchView v, const std::int32_t * group)
{
    __shared__ SegmentFloats tile[kCellBlock];
    __shared__ SegmentBases tileBase[kCellBlock];

    if (cancelRequested(v.cancel)) {
        return;
    }
    const CellPlace place = placeOf(v, group);
    const Extremes x = place.farField ? segmentPasses<Far, Ratio, true>(v, place, tile, tileBase)
                                      : segmentPasses<Far, Ratio, false>(v, place, tile, tileBase);
    if (place.active) {
        writeCell(v, place, x, Guard::Contour, 0.0);
    }
}

struct Need
{
    bool near;
    bool far;
    bool ratio;

    bool operator<(const Need & other) const
    {
        return std::tie(near, far, ratio) < std::tie(other.near, other.far, other.ratio);
    }
};

Need needOf(const std::array<bool, 5> & sheets, Guard guard)
{
    Need need{sheets[0] || sheets[1] || sheets[3], sheets[1], sheets[2] || sheets[4]};
    if (guard != Guard::None) {
        need.near = true;
    }
    return need;
}

template <bool Value>
using Flag = std::integral_constant<bool, Value>;

template <typename Visit>
void withFlags(bool a, bool b, Visit && visit)
{
    const auto second = [&](auto first) {
        if (b) {
            visit(first, Flag<true>{});
        } else {
            visit(first, Flag<false>{});
        }
    };
    if (a) {
        second(Flag<true>{});
    } else {
        second(Flag<false>{});
    }
}

void launchCells(Guard guard, const Need & need, const BatchView & view, const std::int32_t * group,
                 std::size_t groupSize, std::size_t rows, std::size_t columns, const Stream & stream)
{
    const dim3 blocks(blocksFor(columns, kCellBlock), static_cast<unsigned int>(rows), static_cast<unsigned int>(groupSize));
    if (guard == Guard::Contour) {
        withFlags(need.far, need.ratio, [&](auto far, auto ratio) {
            segmentCells<decltype(far)::value, decltype(ratio)::value><<<blocks, kCellBlock, 0, stream.get()>>>(view, group);
        });
    } else if (!need.near) {
        pointCells<false, false, true, false><<<blocks, kCellBlock, 0, stream.get()>>>(view, group);
    } else {
        withFlags(need.far, need.ratio, [&](auto far, auto ratio) {
            if (guard == Guard::Cloud) {
                pointCells<true, decltype(far)::value, decltype(ratio)::value, true>
                        <<<blocks, kCellBlock, 0, stream.get()>>>(view, group);
            } else {
                pointCells<true, decltype(far)::value, decltype(ratio)::value, false>
                        <<<blocks, kCellBlock, 0, stream.get()>>>(view, group);
            }
        });
    }
    checkLaunch("the kernel of the sheets");
}

Guard guardOf(const SheetProblem & problem)
{
    if (!problem.guard) {
        return Guard::None;
    }
    return problem.contours ? Guard::Contour : Guard::Cloud;
}

std::size_t usedSheets(const std::array<bool, 5> & sheets)
{
    return static_cast<std::size_t>(std::count(sheets.begin(), sheets.end(), true));
}

std::size_t bytesPerSample(Guard guard)
{
    std::size_t bytes = 2 * sizeof(DeviceComplex) + sizeof(double) + sizeof(SampleFloats);
    if (guard == Guard::Contour) {
        bytes += sizeof(SegmentFloats) + sizeof(SegmentDoubles);
    }
    if (guard == Guard::Cloud) {
        bytes += sizeof(double);
    }
    return bytes;
}

double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

struct Prepared
{
    std::vector<DeviceComplex> q;
    std::vector<double> radius;
    std::vector<SampleFloats> single;
    std::vector<SegmentFloats> segment;
    std::vector<SegmentDoubles> segmentExact;
    std::vector<double> spacing;
    float scale = 0.0f;
    double largest = 0.0;
};

void buildSegmentsOnProcessor(Prepared & prepared)
{
    const std::int32_t n = static_cast<std::int32_t>(prepared.q.size());
    prepared.segment.resize(prepared.q.size());
    prepared.segmentExact.resize(prepared.q.size());
    std::int32_t start = 0;
    while (start < n) {
        const DeviceComplex head = prepared.q[static_cast<std::size_t>(start)];
        std::int32_t end = n;
        for (std::int32_t j = start + 1; j < n; ++j) {
            if (prepared.q[static_cast<std::size_t>(j)] == head) {
                end = j;
                break;
            }
        }
        const std::int32_t last = end < n ? end : n - 1;
        for (std::int32_t j = start; j <= last; ++j) {
            const DeviceComplex a = prepared.q[static_cast<std::size_t>(j)];
            const bool inLoop = j < end;
            const DeviceComplex b = inLoop ? (j + 1 < end ? prepared.q[static_cast<std::size_t>(j + 1)] : head) : a;
            prepareSegment(a, b, inLoop && j + 1 == end, prepared.segment[static_cast<std::size_t>(j)],
                           prepared.segmentExact[static_cast<std::size_t>(j)]);
        }
        start = end + 1;
    }
}

void spacingsOnProcessor(Prepared & prepared)
{
    const std::size_t n = prepared.q.size();
    const bool everything = !(2.0f * prepared.scale <= kLargest);
    const Shrink shrink = shrinkOf(2.0 * prepared.largest);
    prepared.spacing.assign(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        const SampleFloats & me = prepared.single[i];
        const CellView cell{0, me.x, me.y, me.radius, 0.0f, 0.0f, 0.0f, 0.0f};
        Bounds bounds;
        for (std::size_t j = 0; j < n && !everything; ++j) {
            if (j != i) {
                boundSample<false>(bounds, prepared.single[j], sampleBase<false>(prepared.single[j], cell), cell, true,
                                   false, false);
            }
        }
        double nearest2 = kInfinity;
        for (std::size_t j = 0; j < n; ++j) {
            const Flags f = flagSample<false>(bounds, prepared.single[j], sampleBase<false>(prepared.single[j], cell),
                                              cell, everything, true, false, false);
            if (j != i && f.near) {
                nearest2 = std::min(nearest2, distance2(prepared.q[j], -prepared.q[i], shrink));
            }
        }
        const double nearest = std::sqrt(nearest2) / shrink.factor;
        prepared.spacing[i] = std::isfinite(nearest) ? nearest : 0.0;
    }
}

Prepared prepare(const SheetProblem & problem, std::size_t frequency)
{
    const std::vector<std::complex<double>> & plants = problem.templates->at(frequency);
    const DeviceComplex p0(problem.nominal.at(frequency).real(), problem.nominal.at(frequency).imag());
    Prepared prepared;
    prepared.q.resize(plants.size());
    prepared.radius.resize(plants.size());
    prepared.single.resize(plants.size());
    double largest = 0.0;
    for (std::size_t i = 0; i < plants.size(); ++i) {
        prepareSample(p0, DeviceComplex(plants[i].real(), plants[i].imag()), prepared.q[i], prepared.radius[i],
                      prepared.single[i]);
        const double radius = prepared.radius[i];
        largest = radius <= largest ? largest : (radius == radius ? radius : kInfinity);
    }
    prepared.largest = largest;
    prepared.scale = largest <= 1e38 ? static_cast<float>(largest) * kRadiusUp : kInfinityF;
    const Guard guard = guardOf(problem);
    if (guard == Guard::Contour) {
        buildSegmentsOnProcessor(prepared);
    }
    if (guard == Guard::Cloud) {
        spacingsOnProcessor(prepared);
    }
    return prepared;
}

template <bool FarField>
Extremes pointCellOnProcessor(const Prepared & prepared, DeviceComplex L, const CellView & cell, bool everything,
                              bool indexed, const Shrink & shrink)
{
    const std::size_t n = prepared.q.size();
    Bounds bounds;
    for (std::size_t i = 0; i < n && !everything; ++i) {
        boundSample<FarField>(bounds, prepared.single[i], sampleBase<FarField>(prepared.single[i], cell), cell, true,
                              true, true);
    }
    Extremes x;
    for (std::size_t i = 0; i < n; ++i) {
        const Flags f = flagSample<FarField>(bounds, prepared.single[i], sampleBase<FarField>(prepared.single[i], cell),
                                             cell, everything, true, true, true);
        if (f.near || f.far || f.ratio) {
            if (indexed) {
                keepSample<true>(x, f, prepared.q[i], L, prepared.radius[i], static_cast<std::int32_t>(i), shrink);
            } else {
                keepSample<false>(x, f, prepared.q[i], L, prepared.radius[i], static_cast<std::int32_t>(i), shrink);
            }
        }
    }
    return x;
}

template <bool FarField>
Extremes segmentCellOnProcessor(const Prepared & prepared, DeviceComplex L, const CellView & cell, bool everything)
{
    const std::size_t n = prepared.q.size();
    const DeviceComplex z = -L;
    Bounds bounds;
    for (std::size_t i = 0; i < n && !everything; ++i) {
        boundSegment<FarField>(bounds, prepared.segment[i], segmentBases<FarField>(prepared.segment[i], cell), cell,
                               true, true);
    }
    Extremes x;
    std::int32_t winding = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const SegmentFloats & s = prepared.segment[i];
        const Flags f = flagSegment<FarField>(bounds, s, segmentBases<FarField>(s, cell), cell, everything, true, true);
        if (f.near || f.far || f.ratio) {
            keepSegment<true>(x, f, z, prepared.q[i], prepared.segmentExact[i]);
        }
        if constexpr (!FarField) {
            winding += f.doubtful ? crossingExact(z, prepared.q[i], prepared.segmentExact[i].b) : f.step;
            if (s.closes != 0) {
                x.inside = x.inside || winding != 0;
                winding = 0;
            }
        }
    }
    return x;
}

}

struct SheetsOnProcessor::State
{
    Prepared prepared;
};

SheetsOnProcessor::SheetsOnProcessor(const SheetProblem & problem, std::size_t frequency)
    : m_problem(problem), m_frequency(frequency), m_state(std::make_unique<State>())
{
    m_state->prepared = prepare(problem, frequency);
}

SheetsOnProcessor::~SheetsOnProcessor() = default;

std::array<double, 5> SheetsOnProcessor::cell(std::size_t row, std::size_t column) const
{
    const Prepared & prepared = m_state->prepared;
    const double r = m_problem.rowMagnitudes.at(row);
    const DeviceComplex L(r * m_problem.cosines.at(column), r * m_problem.sines.at(column));
    const CellView view = cellView(L, r, m_problem.cosines.at(column), m_problem.sines.at(column));
    const bool everything = !(prepared.scale + view.zRadius <= kLargest);
    const bool farField = farFieldOf(view.zRadius, prepared.scale);
    const Shrink shrink = shrinkOf(prepared.largest + r);
    const Guard guard = guardOf(m_problem);

    if (prepared.q.empty()) {
        const CellValues empty{};
        return {empty.stabilityNoise, empty.tracking, empty.output, empty.input, empty.effort};
    }
    Extremes x;
    if (guard == Guard::Contour) {
        x = farField ? segmentCellOnProcessor<true>(prepared, L, view, everything)
                     : segmentCellOnProcessor<false>(prepared, L, view, everything);
    } else {
        x = farField ? pointCellOnProcessor<true>(prepared, L, view, everything, guard == Guard::Cloud, shrink)
                     : pointCellOnProcessor<false>(prepared, L, view, everything, guard == Guard::Cloud, shrink);
    }
    const double halfSpacing = guard == Guard::Cloud && x.nearestIndex >= 0
            ? 0.5 * prepared.spacing[static_cast<std::size_t>(x.nearestIndex)] : 0.0;
    const std::complex<double> p0 = m_problem.nominal.at(m_frequency);
    const CellValues values = decibels(magnitudesOf(x, std::hypot(L.real(), L.imag()), std::abs(p0), guard, halfSpacing,
                                                    shrink),
                                       SheetsUsed{true, true, true, true, true});
    return {values.stabilityNoise, values.tracking, values.output, values.input, values.effort};
}

const std::vector<double> & SheetsOnProcessor::spacings() const
{
    return m_state->prepared.spacing;
}

SheetRun::SheetRun(const SheetProblem & problem)
    : problem(problem), rows(problem.rowMagnitudes.size()), columns(problem.cosines.size()), guard(guardOf(problem))
{
    if (rows > 65535) {
        throw CudaError("the Nichols grid has more magnitudes than a launch holds", 0);
    }
    rowMagnitude = uploaded(problem.rowMagnitudes, stream);
    cosine = uploaded(problem.cosines, stream);
    sine = uploaded(problem.sines, stream);
}

std::vector<BatchRange> batchesOf(const SheetRun & run, const std::function<std::size_t(std::size_t)> & extraBytes)
{
    const std::vector<std::vector<std::complex<double>>> & templates = *run.problem.templates;
    const std::size_t cells = run.rows * run.columns;
    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    check(cudaMemGetInfo(&freeBytes, &totalBytes), "cudaMemGetInfo");
    const std::size_t budget = freeBytes / 2;

    std::vector<BatchRange> batches;
    std::size_t first = 0;
    while (first < templates.size()) {
        std::size_t end = first;
        std::size_t bytes = 0;
        while (end < templates.size() && end - first < 65535) {
            const std::size_t more = templates[end].size() * bytesPerSample(run.guard)
                    + usedSheets(run.problem.sheets.at(end)) * cells * sizeof(double) + extraBytes(end);
            if (end > first && bytes + more > budget) {
                break;
            }
            if (more > budget) {
                throw CudaError("the card's free memory cannot hold the sheets of one frequency", 0);
            }
            bytes += more;
            ++end;
        }
        batches.push_back({first, end - first});
        first = end;
    }
    return batches;
}

SheetBatch::SheetBatch(SheetRun & run, BatchRange batch) : range(batch)
{
    const SheetProblem & problem = run.problem;
    const std::vector<std::vector<std::complex<double>>> & templates = *problem.templates;
    const std::size_t count = range.count;
    const std::size_t cells = run.rows * run.columns;
    const Stream & stream = run.stream;

    offset.assign(count, 0);
    sampleCount.assign(count, 0);
    sheetOffset.assign(5 * count, -1);
    std::vector<DeviceComplex> nominal(count);
    std::vector<double> nominalMagnitude(count);
    std::size_t samples = 0;
    std::size_t largestTemplate = 0;
    std::map<Need, std::vector<std::int32_t>> groups;
    for (std::size_t u = 0; u < count; ++u) {
        const std::vector<std::complex<double>> & cloud = templates[range.first + u];
        offset[u] = samples;
        sampleCount[u] = static_cast<std::int32_t>(cloud.size());
        samples += cloud.size();
        largestTemplate = std::max(largestTemplate, cloud.size());
        const std::complex<double> p0 = problem.nominal.at(range.first + u);
        nominal[u] = DeviceComplex(p0.real(), p0.imag());
        nominalMagnitude[u] = std::abs(p0);
        const std::array<bool, 5> & used = problem.sheets.at(range.first + u);
        for (int k = 0; k < 5; ++k) {
            if (used[static_cast<std::size_t>(k)]) {
                sheetOffset[5 * u + static_cast<std::size_t>(k)] = sheetValues;
                sheetValues += static_cast<std::int64_t>(cells);
            }
        }
        if (usedSheets(used) > 0) {
            groups[needOf(used, run.guard)].push_back(static_cast<std::int32_t>(u));
        }
    }
    std::vector<DeviceComplex> plants;
    plants.reserve(samples);
    for (std::size_t u = 0; u < count; ++u) {
        for (const std::complex<double> & p : templates[range.first + u]) {
            plants.emplace_back(p.real(), p.imag());
        }
    }

    m_offset = uploaded(offset, stream);
    m_count = uploaded(sampleCount, stream);
    m_nominal = uploaded(nominal, stream);
    m_nominalMagnitude = uploaded(nominalMagnitude, stream);
    m_sheetOffset = uploaded(sheetOffset, stream);
    m_plants = uploaded(plants, stream);
    m_q = DeviceBuffer<DeviceComplex>(samples, stream);
    m_radius = DeviceBuffer<double>(samples, stream);
    m_single = DeviceBuffer<SampleFloats>(samples, stream);
    m_scale = DeviceBuffer<float>(count, stream);
    m_largest = DeviceBuffer<double>(count, stream);
    m_segment = DeviceBuffer<SegmentFloats>(run.guard == Guard::Contour ? samples : 0, stream);
    m_segmentExact = DeviceBuffer<SegmentDoubles>(run.guard == Guard::Contour ? samples : 0, stream);
    m_spacing = DeviceBuffer<double>(run.guard == Guard::Cloud ? samples : 0, stream);
    m_sheets = DeviceBuffer<double>(static_cast<std::size_t>(sheetValues), stream);

    view.phaseCount = static_cast<std::int32_t>(run.columns);
    view.rowMagnitude = run.rowMagnitude.data();
    view.cosine = run.cosine.data();
    view.sine = run.sine.data();
    view.offset = m_offset.data();
    view.count = m_count.data();
    view.nominal = m_nominal.data();
    view.nominalMagnitude = m_nominalMagnitude.data();
    view.sheetOffset = m_sheetOffset.data();
    view.plants = m_plants.data();
    view.q = m_q.data();
    view.radius = m_radius.data();
    view.single = m_single.data();
    view.scale = m_scale.data();
    view.largest = m_largest.data();
    view.segment = m_segment.data();
    view.segmentExact = m_segmentExact.data();
    view.spacing = m_spacing.data();
    view.sheets = m_sheets.data();
    view.cancel = run.flag.device();

    if (largestTemplate > 0) {
        const dim3 sampleBlocks(blocksFor(largestTemplate, kCellBlock), static_cast<unsigned int>(count));
        prepareSamples<<<sampleBlocks, kCellBlock, 0, stream.get()>>>(view);
        checkLaunch("the preparation of the templates");
        measureScale<<<static_cast<unsigned int>(count), kScaleBlock, 0, stream.get()>>>(view);
        checkLaunch("the scale of the templates");
        if (run.guard == Guard::Contour) {
            buildSegments<<<static_cast<unsigned int>(count), 32, 0, stream.get()>>>(view);
            checkLaunch("the loops of the contours");
        }
        if (run.guard == Guard::Cloud) {
            spacings<<<sampleBlocks, kCellBlock, 0, stream.get()>>>(view);
            checkLaunch("the spacings of the clouds");
        }
    }

    for (const auto & [need, members] : groups) {
        m_groups.push_back(uploaded(members, stream));
        launchCells(run.guard, need, view, m_groups.back().data(), members.size(), run.rows, run.columns, stream);
    }
}

SheetsOnCard sheetsOnCard(const SheetProblem & problem, const CancellationToken * cancellation)
{
    SheetsOnCard result;
    const std::size_t frequencyCount = problem.templates->size();
    result.sheets.assign(frequencyCount, {});
    if (frequencyCount == 0 || problem.rowMagnitudes.empty() || problem.cosines.empty()) {
        return result;
    }

    useProbedDevice();
    SheetRun run(problem);
    const std::size_t rows = run.rows;
    const std::size_t columns = run.columns;

    for (const BatchRange & range : batchesOf(run, [](std::size_t) { return std::size_t{0}; })) {
        const auto computing = std::chrono::steady_clock::now();
        const SheetBatch batch(run, range);
        if (!run.stream.finish(cancellation, &run.flag)) {
            result.cancelled = true;
            return result;
        }
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        PinnedBuffer<double> staging(static_cast<std::size_t>(batch.sheetValues));
        if (batch.sheetValues > 0) {
            check(cudaMemcpyAsync(staging.data(), batch.view.sheets,
                                  static_cast<std::size_t>(batch.sheetValues) * sizeof(double), cudaMemcpyDeviceToHost,
                                  run.stream.get()), "cudaMemcpyAsync of the sheets");
            run.stream.synchronize();
        }
        for (std::size_t u = 0; u < range.count; ++u) {
            BoundarySheets & frequencySheets = result.sheets[range.first + u];
            for (std::size_t k = 0; k < 5; ++k) {
                const std::int64_t at = batch.sheetOffset[5 * u + k];
                if (at < 0) {
                    continue;
                }
                BoundarySheet & sheet = frequencySheets[k];
                sheet.resize(rows);
                for (std::size_t row = 0; row < rows; ++row) {
                    const double * from = staging.data() + at + static_cast<std::int64_t>(row * columns);
                    sheet[row].assign(from, from + columns);
                }
            }
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }
    return result;
}

}
