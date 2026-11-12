/**
 * @file
 * @brief The kernels of the walk's preparation and their run.
 *
 * A batch of frequencies is one array of points, frequency after
 * frequency, and every order is a stable radix sort of all of them,
 * ordered by its least key first and by the frequency last: by angle and
 * by magnitude for MATLAB's order, by phase for the widest gap of the
 * Nichols cut, by cell for the neighbour grid, by rightmost point for the
 * parts and by part for their points. Repeats are dropped by a scan of the
 * starts of runs of equal points, and the roots of the parts by another.
 *
 * The parts are a union-find without locks (Jaiganesh and Burtscher 2018,
 * ECL-CC): a warp takes a point, a thread per column of the grid finds its
 * cells by binary search, and the warp shares out the candidates, joining
 * those within epsilon after the point. A root is always hung under a
 * smaller one, so each part ends named by its first point, as on the
 * processor, whatever order the warps meet the pairs in. Extremes go up by
 * one atomic per block or per warp whose threads agree, on the bits of
 * orderedBits().
 */

#include "src/core/gpu/templates/walk_preparation.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <string>
#include <utility>

#include <cub/block/block_reduce.cuh>
#include <cub/device/device_radix_sort.cuh>
#include <cub/device/device_scan.cuh>
#include <cub/util_type.cuh>
#include <cuda_runtime.h>
#include <math_constants.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/launch.cuh"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/device_clouds.cuh"
#include "src/core/gpu/templates/walk_keys.cuh"

namespace qftbx::gpu {

namespace {

using Bits = unsigned long long;
using Index = std::uint32_t;

constexpr int kThreads = 256;
constexpr int kBoundsThreads = 256;
constexpr int kPartWarps = 4;
constexpr int kPartBlock = 32 * kPartWarps;
constexpr unsigned int kFull = 0xffffffffu;
constexpr Index kNoPoint = 0xffffffffu;
constexpr std::int64_t kCellLimit = (std::int64_t{1} << kCellBits) - 1;
constexpr std::size_t kBytesPerPoint = 2 * sizeof(Bits) + 9 * sizeof(Index) + 2 * sizeof(DeviceComplex)
        + 2 * sizeof(Bits) + sizeof(double);
constexpr std::size_t kLargestBatch = (std::size_t{1} << 31) - 2;

QFTBX_HD inline double valueOfBits(Bits ordered)
{
    const std::uint64_t bits = (ordered >> 63) != 0 ? ordered & ~(std::uint64_t{1} << 63) : ~ordered;
    double value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

int bitsFor(std::size_t count)
{
    int bits = 1;
    while (bits < 32 && (std::size_t{1} << bits) < count) {
        ++bits;
    }
    return bits;
}

__global__ void angleKeys(const DeviceComplex * cloud, Index n, Bits * keys, Index * order)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < n) {
        keys[t] = orderedBits(angleOf(cloud[t]));
        order[t] = t;
    }
}

__global__ void magnitudeKeys(const DeviceComplex * cloud, const Index * order, Index n, Bits * keys)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < n) {
        keys[t] = orderedBits(magnitudeOf(cloud[order[t]]));
    }
}

__global__ void batchFrequencyKeys(const Index * order, Index n, Index plants, Index * keys)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < n) {
        keys[t] = order[t] / plants;
    }
}

__global__ void runStarts(const DeviceComplex * cloud, const Index * order, Index n, Index plants, Index * flags)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < n) {
        flags[t] = t % plants == 0 || !(cloud[order[t]] == cloud[order[t - 1]]) ? 1u : 0u;
    } else if (t == n) {
        flags[t] = 0u;
    }
}

__global__ void compact(const DeviceComplex * cloud, const Index * order, const Index * flags, const Index * places,
                        Index n, Index plants, Index frequencies, DeviceComplex * points, Index * frequencyOf,
                        Index * offsets)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t >= n) {
        return;
    }
    if (flags[t] != 0) {
        points[places[t]] = cloud[order[t]];
        frequencyOf[places[t]] = t / plants;
    }
    if (t % plants == 0) {
        offsets[t / plants] = places[t];
    }
    if (t == 0) {
        offsets[frequencies] = places[n];
    }
}

__global__ void phaseKeys(const DeviceComplex * points, Index m, Bits * keys, Index * order)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        keys[t] = orderedBits(phaseOf(points[t]));
        order[t] = t;
    }
}

__global__ void frequencyKeys(const Index * order, const Index * frequencyOf, Index m, Index * keys)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        keys[t] = frequencyOf[order[t]];
    }
}

__global__ void sortedPhases(const DeviceComplex * points, const Index * order, Index m, double * phases)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        phases[t] = phaseOf(points[order[t]]);
    }
}

__global__ void widestGaps(const double * phases, const Index * order, const Index * frequencyOf,
                           const Index * offsets, Index m, Bits * widest)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        const Index f = frequencyOf[order[t]];
        if (t > offsets[f]) {
            atomicMax(widest + f, static_cast<Bits>(orderedBits(phases[t] - phases[t - 1])));
        }
    }
}

__global__ void firstWidest(const double * phases, const Index * order, const Index * frequencyOf,
                            const Index * offsets, const Bits * widest, Index m, Index * at)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        const Index f = frequencyOf[order[t]];
        if (t > offsets[f] && orderedBits(phases[t] - phases[t - 1]) == widest[f]) {
            atomicMin(at + f, t);
        }
    }
}

__global__ void phaseCuts(const double * phases, const Index * offsets, const Index * at, Index frequencies,
                          int * shift, double * cut)
{
    const Index f = blockIdx.x * blockDim.x + threadIdx.x;
    if (f < frequencies) {
        const Index start = offsets[f];
        const Index end = offsets[f + 1];
        const double wrap = phases[start] + 360.0 - phases[end - 1];
        const bool gaps = end - start >= 2;
        const bool below = gaps && phases[at[f]] - phases[at[f] - 1] > wrap;
        shift[f] = below ? 1 : 0;
        cut[f] = below ? phases[at[f] - 1] : 0.0;
    }
}

__global__ void projectNichols(const DeviceComplex * points, const Index * frequencyOf, const int * shift,
                               const double * cut, double dbPerDegree, Index m, DeviceComplex * walk)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        const Index f = frequencyOf[t];
        walk[t] = nicholsPoint(points[t], dbPerDegree, shift[f] != 0, cut[f]);
    }
}

struct Bounds
{
    double minRe;
    double maxRe;
    double minIm;
    double maxIm;
};

struct WiderBounds
{
    __device__ Bounds operator()(const Bounds & a, const Bounds & b) const
    {
        return {::fmin(a.minRe, b.minRe), ::fmax(a.maxRe, b.maxRe), ::fmin(a.minIm, b.minIm),
                ::fmax(a.maxIm, b.maxIm)};
    }
};

__global__ void __launch_bounds__(kBoundsThreads) boundsOf(const DeviceComplex * walk, const Index * offsets,
                                                           Bits * low, Bits * high)
{
    const Index f = blockIdx.y;
    const Index end = offsets[f + 1];
    Bounds own{CUDART_INF, -CUDART_INF, CUDART_INF, -CUDART_INF};
    for (Index i = offsets[f] + blockIdx.x * blockDim.x + threadIdx.x; i < end; i += gridDim.x * blockDim.x) {
        const DeviceComplex z = walk[i];
        own = WiderBounds()(own, Bounds{z.real(), z.real(), z.imag(), z.imag()});
    }
    using Reduce = cub::BlockReduce<Bounds, kBoundsThreads>;
    __shared__ typename Reduce::TempStorage storage;
    const Bounds all = Reduce(storage).Reduce(own, WiderBounds());
    if (threadIdx.x == 0 && all.minRe <= all.maxRe) {
        atomicMin(low + 2 * f, static_cast<Bits>(orderedBits(all.minRe)));
        atomicMin(low + 2 * f + 1, static_cast<Bits>(orderedBits(all.minIm)));
        atomicMax(high + 2 * f, static_cast<Bits>(orderedBits(all.maxRe)));
        atomicMax(high + 2 * f + 1, static_cast<Bits>(orderedBits(all.maxIm)));
    }
}

__global__ void gridFrames(const Bits * low, const Bits * high, const double * epsilon, Index frequencies,
                           GridFrame * frames)
{
    const Index f = blockIdx.x * blockDim.x + threadIdx.x;
    if (f < frequencies) {
        frames[f] = gridFrame(valueOfBits(low[2 * f]), valueOfBits(high[2 * f]), valueOfBits(low[2 * f + 1]),
                              valueOfBits(high[2 * f + 1]), epsilon[f]);
    }
}

__global__ void cellKeys(const DeviceComplex * walk, const Index * frequencyOf, const GridFrame * frames, Index m,
                         Bits * keys, Index * order)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        keys[t] = cellKeyOf(frames[frequencyOf[t]], walk[t]);
        order[t] = t;
    }
}

__global__ void sortedCells(const DeviceComplex * walk, const Index * frequencyOf, const GridFrame * frames,
                            const Index * order, Index m, Bits * cells)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        const Index point = order[t];
        cells[t] = cellKeyOf(frames[frequencyOf[point]], walk[point]);
    }
}

__device__ Index lowerBound(const Bits * keys, Index first, Index last, Bits key)
{
    while (first < last) {
        const Index middle = first + (last - first) / 2;
        if (keys[middle] < key) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first;
}

__device__ Index upperBound(const Bits * keys, Index first, Index last, Bits key)
{
    while (first < last) {
        const Index middle = first + (last - first) / 2;
        if (keys[middle] <= key) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first;
}

__device__ Index representative(Index * parent, Index x)
{
    Index current = __ldcg(parent + x);
    if (current != x) {
        Index previous = x;
        Index next;
        while (current > (next = __ldcg(parent + current))) {
            __stcg(parent + previous, next);
            previous = current;
            current = next;
        }
    }
    return current;
}

__device__ void unite(Index * parent, Index a, Index b)
{
    Index low = representative(parent, a);
    Index high = representative(parent, b);
    while (low != high) {
        if (low > high) {
            const Index swapped = low;
            low = high;
            high = swapped;
        }
        const Index seen = atomicCAS(parent + high, high, low);
        if (seen == high) {
            return;
        }
        high = seen;
    }
}

__global__ void initialParents(Index m, Index * parent)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        parent[t] = t;
    }
}

__global__ void __launch_bounds__(kPartBlock) uniteParts(const DeviceComplex * walk, const Index * frequencyOf,
                                                         const Index * offsets, const GridFrame * frames,
                                                         const double * epsilon, const Bits * cells,
                                                         const Index * cellOrder, Index m, Index * parent)
{
    const int lane = static_cast<int>(threadIdx.x % 32);
    const Index warps = gridDim.x * kPartWarps;
    for (Index i = blockIdx.x * kPartWarps + threadIdx.x / 32; i < m; i += warps) {
        const Index f = frequencyOf[i];
        const Index start = offsets[f];
        const Index end = offsets[f + 1];
        const GridFrame frame = frames[f];
        const double reach = epsilon[f];
        const DeviceComplex centre = walk[i];

        Index lo = 0;
        Index hi = 0;
        if (frame.indexed) {
            const std::int64_t x = cellCoordinate(centre.real(), frame.minRe, frame.side) - kCellReach + lane;
            const std::int64_t y = cellCoordinate(centre.imag(), frame.minIm, frame.side);
            if (lane < kCellColumns && x >= 0 && x <= kCellLimit) {
                const std::int64_t below = y - kCellReach < 0 ? 0 : y - kCellReach;
                const std::int64_t above = y + kCellReach > kCellLimit ? kCellLimit : y + kCellReach;
                lo = lowerBound(cells, start, end, static_cast<Bits>(cellKey(x, below)));
                hi = upperBound(cells, lo, end, static_cast<Bits>(cellKey(x, above)));
            }
        } else if (lane == 0) {
            lo = start;
            hi = end;
        }

        const Index count = hi - lo;
        Index inclusive = count;
        for (int d = 1; d < 32; d <<= 1) {
            const Index below = __shfl_up_sync(kFull, inclusive, d);
            inclusive += lane >= d ? below : 0u;
        }
        const Index exclusive = inclusive - count;
        const Index total = __shfl_sync(kFull, inclusive, 31);

        for (Index base = 0; base < total; base += 32) {
            const Index c = base + static_cast<Index>(lane);
            Index place = 0;
            for (int k = 0; k < kCellColumns; ++k) {
                const Index from = __shfl_sync(kFull, exclusive, k);
                const Index first = __shfl_sync(kFull, lo, k);
                place = c >= from ? first + (c - from) : place;
            }
            const bool valid = c < total;
            const Index j = valid ? cellOrder[place] : 0u;
            if (valid && j > i && reaches(walk[j], centre, reach)) {
                unite(parent, i, j);
            }
        }
    }
}

__global__ void finishLabels(Index * parent, Index m, Index * label)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        label[t] = representative(parent, t);
    }
}

__global__ void rightmostOf(const DeviceComplex * walk, const Index * label, Index m, Bits * rightmost)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    const bool in = t < m;
    const Index root = in ? label[t] : kNoPoint;
    Bits bits = in ? static_cast<Bits>(orderedBits(walk[t].real())) : 0ull;
    const Index lead = __shfl_sync(kFull, root, 0);
    if (__all_sync(kFull, root == lead) && lead != kNoPoint) {
        for (int d = 16; d > 0; d >>= 1) {
            const Bits other = __shfl_down_sync(kFull, bits, d);
            bits = other > bits ? other : bits;
        }
        if (threadIdx.x % 32 == 0) {
            atomicMax(rightmost + lead, bits);
        }
    } else if (in) {
        atomicMax(rightmost + root, bits);
    }
}

__global__ void rootFlags(const Index * label, Index m, Index * flags)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t <= m) {
        flags[t] = t < m && label[t] == t ? 1u : 0u;
    }
}

__global__ void rootKeys(const Index * label, const Index * places, const Bits * rightmost, Index m, Bits * keys,
                         Index * roots)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m && label[t] == t) {
        keys[places[t]] = ~rightmost[t];
        roots[places[t]] = t;
    }
}

__global__ void ranks(const Index * roots, Index r, Index * rankOf)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < r) {
        rankOf[roots[t]] = t;
    }
}

__global__ void memberKeys(const Index * label, const Index * rankOf, Index m, Index * keys, Index * members)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        keys[t] = rankOf[label[t]];
        members[t] = t;
    }
}

__global__ void partStarts(const Index * keys, Index m, Index * starts)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m && (t == 0 || keys[t] != keys[t - 1])) {
        starts[keys[t]] = t;
    }
}

__global__ void rootOffsetsOf(const Index * offsets, const Index * places, Index frequencies, Index * rootOffsets)
{
    const Index f = blockIdx.x * blockDim.x + threadIdx.x;
    if (f <= frequencies) {
        rootOffsets[f] = places[offsets[f]];
    }
}

unsigned int blocks(std::size_t threads)
{
    return std::max(1u, blocksFor(threads, kThreads));
}

template <typename Kernel>
unsigned int residentBlocks(Kernel kernel, int block, std::size_t needed)
{
    int perMultiprocessor = 0;
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perMultiprocessor, kernel, block, 0),
          "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
    const std::size_t resident = static_cast<std::size_t>(std::max(perMultiprocessor, 1))
            * static_cast<std::size_t>(std::max(probe().multiprocessors, 1));
    return static_cast<unsigned int>(std::max<std::size_t>(1, std::min(resident, needed)));
}

class Sorter
{
public:
    Sorter(std::size_t items, const Stream & stream) : m_stream(stream)
    {
        const int n = static_cast<int>(items);
        std::size_t bytes = 0;
        std::size_t largest = 0;
        cub::DoubleBuffer<Bits> wide(nullptr, nullptr);
        cub::DoubleBuffer<Index> narrow(nullptr, nullptr);
        cub::DoubleBuffer<Index> values(nullptr, nullptr);
        check(cub::DeviceRadixSort::SortPairs(nullptr, bytes, wide, values, n), "cub::DeviceRadixSort::SortPairs");
        largest = std::max(largest, bytes);
        check(cub::DeviceRadixSort::SortPairs(nullptr, bytes, narrow, values, n), "cub::DeviceRadixSort::SortPairs");
        largest = std::max(largest, bytes);
        check(cub::DeviceScan::ExclusiveSum(nullptr, bytes, static_cast<const Index *>(nullptr),
                                            static_cast<Index *>(nullptr), n + 1),
              "cub::DeviceScan::ExclusiveSum");
        largest = std::max(largest, bytes);
        m_bytes = largest;
        m_storage = DeviceBuffer<unsigned char>(std::max<std::size_t>(largest, 1), stream);
    }

    template <typename Key>
    void sort(Key *& keys, Key * spareKeys, Index *& values, Index *& spareValues, Index n, int bits)
    {
        if (n == 0) {
            return;
        }
        cub::DoubleBuffer<Key> k(keys, spareKeys);
        cub::DoubleBuffer<Index> v(values, spareValues);
        std::size_t bytes = m_bytes;
        check(cub::DeviceRadixSort::SortPairs(m_storage.data(), bytes, k, v, static_cast<int>(n), 0, bits,
                                              m_stream.get()),
              "cub::DeviceRadixSort::SortPairs");
        keys = k.Current();
        values = v.Current();
        spareValues = v.Alternate();
    }

    void scan(const Index * in, Index * out, Index n)
    {
        std::size_t bytes = m_bytes;
        check(cub::DeviceScan::ExclusiveSum(m_storage.data(), bytes, in, out, static_cast<int>(n), m_stream.get()),
              "cub::DeviceScan::ExclusiveSum");
    }

private:
    const Stream & m_stream;
    std::size_t m_bytes = 0;
    DeviceBuffer<unsigned char> m_storage;
};

struct Work
{
    Work(std::size_t points, std::size_t frequencies, bool nichols, const Stream & stream)
        : keysA(points, stream), keysB(points, stream), valuesA(points, stream), valuesB(points, stream),
          flags(points + 1, stream), places(points + 1, stream), cloudPoints(points, stream), walk(points, stream),
          frequencyOf(points, stream), parent(points, stream), label(points, stream), rankOf(points, stream),
          starts(points, stream), rightmost(points, stream), phases(nichols ? points : 0, stream),
          offsets(frequencies + 1, stream), rootOffsets(frequencies + 1, stream), epsilon(frequencies, stream),
          frames(frequencies, stream), low(2 * frequencies, stream), high(2 * frequencies, stream),
          widest(frequencies, stream), at(frequencies, stream), shift(frequencies, stream), cut(frequencies, stream),
          sorter(points, stream)
    {
    }

    DeviceBuffer<Bits> keysA;
    DeviceBuffer<Bits> keysB;
    DeviceBuffer<Index> valuesA;
    DeviceBuffer<Index> valuesB;
    DeviceBuffer<Index> flags;
    DeviceBuffer<Index> places;
    DeviceBuffer<DeviceComplex> cloudPoints;
    DeviceBuffer<DeviceComplex> walk;
    DeviceBuffer<Index> frequencyOf;
    DeviceBuffer<Index> parent;
    DeviceBuffer<Index> label;
    DeviceBuffer<Index> rankOf;
    DeviceBuffer<Index> starts;
    DeviceBuffer<Bits> rightmost;
    DeviceBuffer<double> phases;
    DeviceBuffer<Index> offsets;
    DeviceBuffer<Index> rootOffsets;
    DeviceBuffer<double> epsilon;
    DeviceBuffer<GridFrame> frames;
    DeviceBuffer<Bits> low;
    DeviceBuffer<Bits> high;
    DeviceBuffer<Bits> widest;
    DeviceBuffer<Index> at;
    DeviceBuffer<int> shift;
    DeviceBuffer<double> cut;
    Sorter sorter;
};

Index * narrowed(DeviceBuffer<Bits> & keys)
{
    return reinterpret_cast<Index *>(keys.data());
}

double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

Index sortedWithoutRepeats(Work & work, const DeviceComplex * cloud, Index n, Index plants, Index frequencies,
                           cudaStream_t stream)
{
    Bits * keys = work.keysA.data();
    Index * order = work.valuesA.data();
    Index * spare = work.valuesB.data();
    angleKeys<<<blocks(n), kThreads, 0, stream>>>(cloud, n, keys, order);
    checkLaunch("the kernel of the angles");
    work.sorter.sort(keys, work.keysB.data(), order, spare, n, 64);

    keys = work.keysA.data();
    magnitudeKeys<<<blocks(n), kThreads, 0, stream>>>(cloud, order, n, keys);
    checkLaunch("the kernel of the magnitudes");
    work.sorter.sort(keys, work.keysB.data(), order, spare, n, 64);

    Index * frequencyKeys = narrowed(work.keysA);
    batchFrequencyKeys<<<blocks(n), kThreads, 0, stream>>>(order, n, plants, frequencyKeys);
    checkLaunch("the kernel of the frequencies");
    work.sorter.sort(frequencyKeys, narrowed(work.keysB), order, spare, n, bitsFor(frequencies));

    runStarts<<<blocks(n + 1), kThreads, 0, stream>>>(cloud, order, n, plants, work.flags.data());
    checkLaunch("the kernel of the repeats");
    work.sorter.scan(work.flags.data(), work.places.data(), n + 1);
    compact<<<blocks(n), kThreads, 0, stream>>>(cloud, order, work.flags.data(), work.places.data(), n, plants,
                                                frequencies, work.cloudPoints.data(), work.frequencyOf.data(),
                                                work.offsets.data());
    checkLaunch("the kernel of the compaction");

    Index points = 0;
    check(cudaMemcpyAsync(&points, work.offsets.data() + frequencies, sizeof points, cudaMemcpyDeviceToHost, stream),
          "cudaMemcpyAsync of the count of points");
    check(cudaStreamSynchronize(stream), "cudaStreamSynchronize");
    return points;
}

void projectOntoNichols(Work & work, Index m, Index frequencies, double dbPerDegree, cudaStream_t stream)
{
    Bits * keys = work.keysA.data();
    Index * order = work.valuesA.data();
    Index * spare = work.valuesB.data();
    phaseKeys<<<blocks(m), kThreads, 0, stream>>>(work.cloudPoints.data(), m, keys, order);
    checkLaunch("the kernel of the phases");
    work.sorter.sort(keys, work.keysB.data(), order, spare, m, 64);
    Index * frequencyKeysOf = narrowed(work.keysA);
    frequencyKeys<<<blocks(m), kThreads, 0, stream>>>(order, work.frequencyOf.data(), m, frequencyKeysOf);
    checkLaunch("the kernel of the frequencies of the phases");
    work.sorter.sort(frequencyKeysOf, narrowed(work.keysB), order, spare, m, bitsFor(frequencies));

    sortedPhases<<<blocks(m), kThreads, 0, stream>>>(work.cloudPoints.data(), order, m, work.phases.data());
    checkLaunch("the kernel of the sorted phases");
    work.widest.fill(0);
    work.at.fill(0xff);
    widestGaps<<<blocks(m), kThreads, 0, stream>>>(work.phases.data(), order, work.frequencyOf.data(),
                                                   work.offsets.data(), m, work.widest.data());
    checkLaunch("the kernel of the widest gaps");
    firstWidest<<<blocks(m), kThreads, 0, stream>>>(work.phases.data(), order, work.frequencyOf.data(),
                                                    work.offsets.data(), work.widest.data(), m, work.at.data());
    checkLaunch("the kernel of the first widest gaps");
    phaseCuts<<<blocks(frequencies), kThreads, 0, stream>>>(work.phases.data(), work.offsets.data(), work.at.data(),
                                                            frequencies, work.shift.data(), work.cut.data());
    checkLaunch("the kernel of the phase cuts");
    projectNichols<<<blocks(m), kThreads, 0, stream>>>(work.cloudPoints.data(), work.frequencyOf.data(),
                                                       work.shift.data(), work.cut.data(), dbPerDegree, m,
                                                       work.walk.data());
    checkLaunch("the kernel of the Nichols projection");
}

Index splitIntoParts(Work & work, Index m, Index plants, Index frequencies, cudaStream_t stream, Index *& members)
{
    work.low.fill(0xff);
    work.high.fill(0);
    const dim3 boundsGrid(std::max(1u, blocksFor(plants, kBoundsThreads * 8)), frequencies);
    boundsOf<<<boundsGrid, kBoundsThreads, 0, stream>>>(work.walk.data(), work.offsets.data(), work.low.data(),
                                                        work.high.data());
    checkLaunch("the kernel of the bounds");
    gridFrames<<<blocks(frequencies), kThreads, 0, stream>>>(work.low.data(), work.high.data(), work.epsilon.data(),
                                                             frequencies, work.frames.data());
    checkLaunch("the kernel of the grid frames");

    Bits * keys = work.keysA.data();
    Index * order = work.valuesA.data();
    Index * spare = work.valuesB.data();
    cellKeys<<<blocks(m), kThreads, 0, stream>>>(work.walk.data(), work.frequencyOf.data(), work.frames.data(), m,
                                                 keys, order);
    checkLaunch("the kernel of the cells");
    work.sorter.sort(keys, work.keysB.data(), order, spare, m, 2 * kCellBits);
    Index * frequencyKeysOf = narrowed(work.keysA);
    frequencyKeys<<<blocks(m), kThreads, 0, stream>>>(order, work.frequencyOf.data(), m, frequencyKeysOf);
    checkLaunch("the kernel of the frequencies of the cells");
    work.sorter.sort(frequencyKeysOf, narrowed(work.keysB), order, spare, m, bitsFor(frequencies));
    Bits * cells = work.keysA.data();
    sortedCells<<<blocks(m), kThreads, 0, stream>>>(work.walk.data(), work.frequencyOf.data(), work.frames.data(),
                                                    order, m, cells);
    checkLaunch("the kernel of the sorted cells");

    initialParents<<<blocks(m), kThreads, 0, stream>>>(m, work.parent.data());
    checkLaunch("the kernel of the first parents");
    const unsigned int partBlocks = residentBlocks(uniteParts, kPartBlock, (m + kPartWarps - 1) / kPartWarps);
    uniteParts<<<partBlocks, kPartBlock, 0, stream>>>(work.walk.data(), work.frequencyOf.data(), work.offsets.data(),
                                                      work.frames.data(), work.epsilon.data(), cells, order, m,
                                                      work.parent.data());
    checkLaunch("the kernel of the parts");
    finishLabels<<<blocks(m), kThreads, 0, stream>>>(work.parent.data(), m, work.label.data());
    checkLaunch("the kernel of the labels");

    work.rightmost.fill(0);
    rightmostOf<<<blocks(m), kThreads, 0, stream>>>(work.walk.data(), work.label.data(), m, work.rightmost.data());
    checkLaunch("the kernel of the rightmost points");
    rootFlags<<<blocks(m + 1), kThreads, 0, stream>>>(work.label.data(), m, work.flags.data());
    checkLaunch("the kernel of the roots");
    work.sorter.scan(work.flags.data(), work.places.data(), m + 1);
    Index roots = 0;
    check(cudaMemcpyAsync(&roots, work.places.data() + m, sizeof roots, cudaMemcpyDeviceToHost, stream),
          "cudaMemcpyAsync of the count of parts");
    check(cudaStreamSynchronize(stream), "cudaStreamSynchronize");

    keys = work.keysA.data();
    Index * rootList = work.valuesA.data();
    spare = work.valuesB.data();
    rootKeys<<<blocks(m), kThreads, 0, stream>>>(work.label.data(), work.places.data(), work.rightmost.data(), m,
                                                 keys, rootList);
    checkLaunch("the kernel of the keys of the parts");
    work.sorter.sort(keys, work.keysB.data(), rootList, spare, roots, 64);
    frequencyKeysOf = narrowed(work.keysA);
    frequencyKeys<<<blocks(roots), kThreads, 0, stream>>>(rootList, work.frequencyOf.data(), roots, frequencyKeysOf);
    checkLaunch("the kernel of the frequencies of the parts");
    work.sorter.sort(frequencyKeysOf, narrowed(work.keysB), rootList, spare, roots, bitsFor(frequencies));
    ranks<<<blocks(roots), kThreads, 0, stream>>>(rootList, roots, work.rankOf.data());
    checkLaunch("the kernel of the ranks");
    rootOffsetsOf<<<blocks(frequencies + 1), kThreads, 0, stream>>>(work.offsets.data(), work.places.data(),
                                                                    frequencies, work.rootOffsets.data());
    checkLaunch("the kernel of the offsets of the parts");

    Index * memberRanks = narrowed(work.keysA);
    members = work.valuesA.data();
    spare = work.valuesB.data();
    memberKeys<<<blocks(m), kThreads, 0, stream>>>(work.label.data(), work.rankOf.data(), m, memberRanks, members);
    checkLaunch("the kernel of the members");
    work.sorter.sort(memberRanks, narrowed(work.keysB), members, spare, m, bitsFor(roots));
    partStarts<<<blocks(m), kThreads, 0, stream>>>(memberRanks, m, work.starts.data());
    checkLaunch("the kernel of the starts of the parts");
    return roots;
}

}

std::shared_ptr<const DeviceClouds> cloudsOnCard(const CloudSet & clouds)
{
    if (clouds.empty() || clouds.front().empty()) {
        return nullptr;
    }
    const std::size_t plants = clouds.front().size();
    for (const ComplexCloud & cloud : clouds) {
        if (cloud.size() != plants) {
            return nullptr;
        }
    }

    useProbedDevice();
    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    check(cudaMemGetInfo(&freeBytes, &totalBytes), "cudaMemGetInfo");
    if (clouds.size() * plants * sizeof(DeviceComplex) > freeBytes / 2) {
        return nullptr;
    }

    auto stream = std::make_shared<Stream>();
    DeviceBuffer<DeviceComplex> values(clouds.size() * plants, *stream);
    for (std::size_t f = 0; f < clouds.size(); ++f) {
        check(cudaMemcpyAsync(values.data() + f * plants, clouds[f].data(), plants * sizeof(DeviceComplex),
                              cudaMemcpyHostToDevice, stream->get()), "cudaMemcpyAsync of the clouds");
    }
    stream->synchronize();
    return std::make_shared<const DeviceClouds>(stream, std::move(values), clouds.size(), plants);
}

PreparedOnCard preparedOnCard(const DeviceClouds & clouds, const std::vector<double> & epsilon, HullMetric metric,
                              double dbPerDegree, const CancellationToken * cancellation)
{
    PreparedOnCard result;
    const std::size_t frequencyCount = clouds.frequencies();
    const std::size_t plants = clouds.plants();
    result.clouds.assign(frequencyCount, {});
    if (frequencyCount == 0 || plants == 0) {
        return result;
    }
    if (epsilon.size() < frequencyCount) {
        throw CudaError("the clouds were given " + std::to_string(epsilon.size()) + " epsilons for "
                        + std::to_string(frequencyCount) + " frequencies", 0);
    }

    useProbedDevice();
    const Stream & stream = clouds.stream();
    const bool nichols = metric == HullMetric::Nichols;

    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    check(cudaMemGetInfo(&freeBytes, &totalBytes), "cudaMemGetInfo");
    const std::size_t perFrequency = plants * kBytesPerPoint * 2;
    if (perFrequency > freeBytes / 2 || plants > kLargestBatch) {
        throw CudaError("the card's free memory cannot hold the preparation of one frequency", 0);
    }
    const std::size_t batch = std::min<std::size_t>({frequencyCount, (freeBytes / 2) / perFrequency,
                                                     kLargestBatch / plants, 65535});

    Work work(batch * plants, batch, nichols, stream);

    for (std::size_t first = 0; first < frequencyCount; first += batch) {
        const Index count = static_cast<Index>(std::min(batch, frequencyCount - first));
        const Index n = static_cast<Index>(count * plants);
        const DeviceComplex * cloud = clouds.values() + first * plants;

        const auto computing = std::chrono::steady_clock::now();
        check(cudaMemcpyAsync(work.epsilon.data(), epsilon.data() + first, count * sizeof(double),
                              cudaMemcpyHostToDevice, stream.get()), "cudaMemcpyAsync of the epsilons");
        const Index m = sortedWithoutRepeats(work, cloud, n, static_cast<Index>(plants), count, stream.get());
        if (nichols) {
            projectOntoNichols(work, m, count, dbPerDegree, stream.get());
        } else {
            check(cudaMemcpyAsync(work.walk.data(), work.cloudPoints.data(), m * sizeof(DeviceComplex),
                                  cudaMemcpyDeviceToDevice, stream.get()), "cudaMemcpyAsync of the walk");
        }
        Index * members = nullptr;
        const Index roots = splitIntoParts(work, m, static_cast<Index>(plants), count, stream.get(), members);
        if (!stream.finish(cancellation, nullptr)) {
            result.cancelled = true;
            return result;
        }
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        std::vector<DeviceComplex> points(m);
        std::vector<DeviceComplex> walk(m);
        std::vector<Index> memberList(m);
        std::vector<Index> starts(roots);
        std::vector<Index> offsets(count + 1);
        std::vector<Index> rootOffsets(count + 1);
        const auto download = [&](void * to, const void * from, std::size_t bytes, const char * what) {
            if (bytes != 0) {
                check(cudaMemcpyAsync(to, from, bytes, cudaMemcpyDeviceToHost, stream.get()), what);
            }
        };
        download(points.data(), work.cloudPoints.data(), m * sizeof(DeviceComplex), "cudaMemcpyAsync of the points");
        download(walk.data(), work.walk.data(), m * sizeof(DeviceComplex), "cudaMemcpyAsync of the walk");
        download(memberList.data(), members, m * sizeof(Index), "cudaMemcpyAsync of the members");
        download(starts.data(), work.starts.data(), roots * sizeof(Index), "cudaMemcpyAsync of the starts");
        download(offsets.data(), work.offsets.data(), (count + 1) * sizeof(Index), "cudaMemcpyAsync of the offsets");
        download(rootOffsets.data(), work.rootOffsets.data(), (count + 1) * sizeof(Index),
                 "cudaMemcpyAsync of the offsets of the parts");
        stream.synchronize();

        for (Index f = 0; f < count; ++f) {
            PreparedCloud & out = result.clouds[first + f];
            const Index from = offsets[f];
            const Index to = offsets[f + 1];
            out.sorted.points.resize(to - from);
            out.sorted.walk.resize(to - from);
            std::memcpy(static_cast<void *>(out.sorted.points.data()), points.data() + from,
                        (to - from) * sizeof(DeviceComplex));
            std::memcpy(static_cast<void *>(out.sorted.walk.data()), walk.data() + from,
                        (to - from) * sizeof(DeviceComplex));
            out.parts.members.resize(to - from);
            for (Index k = from; k < to; ++k) {
                out.parts.members[k - from] = static_cast<std::int32_t>(memberList[k] - from);
            }
            for (Index r = rootOffsets[f]; r < rootOffsets[f + 1]; ++r) {
                out.parts.starts.push_back(starts[r] - from);
            }
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }
    return result;
}

std::vector<PreparedCloud> preparedOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                               HullMetric metric, double dbPerDegree)
{
    std::vector<PreparedCloud> prepared(clouds.size());
    for (std::size_t f = 0; f < clouds.size(); ++f) {
        const ComplexCloud & cloud = clouds[f];
        const std::size_t n = cloud.size();
        PreparedCloud & out = prepared[f];
        if (n == 0) {
            continue;
        }

        std::vector<Index> order(n);
        std::iota(order.begin(), order.end(), 0u);
        std::vector<Bits> angle(n);
        std::vector<Bits> magnitude(n);
        for (std::size_t t = 0; t < n; ++t) {
            const DeviceComplex z(cloud[t].real(), cloud[t].imag());
            angle[t] = orderedBits(angleOf(z));
            magnitude[t] = orderedBits(magnitudeOf(z));
        }
        std::stable_sort(order.begin(), order.end(), [&](Index a, Index b) {
            return magnitude[a] != magnitude[b] ? magnitude[a] < magnitude[b] : angle[a] < angle[b];
        });
        for (std::size_t t = 0; t < n; ++t) {
            if (t == 0 || !(cloud[order[t]] == cloud[order[t - 1]])) {
                out.sorted.points.push_back(cloud[order[t]]);
            }
        }
        const std::size_t m = out.sorted.points.size();

        if (metric == HullMetric::Nichols) {
            std::vector<double> phases(m);
            for (std::size_t i = 0; i < m; ++i) {
                phases[i] = phaseOf(DeviceComplex(out.sorted.points[i].real(), out.sorted.points[i].imag()));
            }
            std::stable_sort(phases.begin(), phases.end(), [](double a, double b) {
                return orderedBits(a) < orderedBits(b);
            });
            Bits widest = 0;
            for (std::size_t t = 1; t < m; ++t) {
                widest = std::max<Bits>(widest, orderedBits(phases[t] - phases[t - 1]));
            }
            std::size_t at = m;
            for (std::size_t t = 1; t < m && at == m; ++t) {
                if (orderedBits(phases[t] - phases[t - 1]) == widest) {
                    at = t;
                }
            }
            const double wrap = phases.front() + 360.0 - phases.back();
            const bool shift = m >= 2 && phases[at] - phases[at - 1] > wrap;
            const double cut = shift ? phases[at - 1] : 0.0;
            for (const std::complex<double> & z : out.sorted.points) {
                const DeviceComplex w = nicholsPoint(DeviceComplex(z.real(), z.imag()), dbPerDegree, shift, cut);
                out.sorted.walk.emplace_back(w.real(), w.imag());
            }
        } else {
            out.sorted.walk = out.sorted.points;
        }
        const ComplexCloud & walk = out.sorted.walk;

        double minRe = walk[0].real();
        double maxRe = walk[0].real();
        double minIm = walk[0].imag();
        double maxIm = walk[0].imag();
        for (const std::complex<double> & w : walk) {
            minRe = std::min(minRe, w.real());
            maxRe = std::max(maxRe, w.real());
            minIm = std::min(minIm, w.imag());
            maxIm = std::max(maxIm, w.imag());
        }
        const GridFrame frame = gridFrame(minRe, maxRe, minIm, maxIm, epsilon.at(f));

        std::vector<Bits> cellOf(m);
        std::vector<Index> cellOrder(m);
        for (std::size_t i = 0; i < m; ++i) {
            cellOf[i] = cellKeyOf(frame, DeviceComplex(walk[i].real(), walk[i].imag()));
        }
        std::iota(cellOrder.begin(), cellOrder.end(), 0u);
        std::stable_sort(cellOrder.begin(), cellOrder.end(), [&](Index a, Index b) { return cellOf[a] < cellOf[b]; });
        std::vector<Bits> cells(m);
        for (std::size_t t = 0; t < m; ++t) {
            cells[t] = cellOf[cellOrder[t]];
        }

        std::vector<Index> parent(m);
        std::iota(parent.begin(), parent.end(), 0u);
        const auto find = [&parent](Index x) {
            while (parent[x] != x) {
                parent[x] = parent[parent[x]];
                x = parent[x];
            }
            return x;
        };
        for (Index i = 0; i < m; ++i) {
            const DeviceComplex centre(walk[i].real(), walk[i].imag());
            std::vector<std::pair<Index, Index>> ranges;
            if (frame.indexed) {
                const std::int64_t x0 = cellCoordinate(centre.real(), frame.minRe, frame.side);
                const std::int64_t y = cellCoordinate(centre.imag(), frame.minIm, frame.side);
                for (int lane = 0; lane < kCellColumns; ++lane) {
                    const std::int64_t x = x0 - kCellReach + lane;
                    if (x < 0 || x > kCellLimit) {
                        continue;
                    }
                    const std::int64_t below = std::max<std::int64_t>(y - kCellReach, 0);
                    const std::int64_t above = std::min<std::int64_t>(y + kCellReach, kCellLimit);
                    const auto lo = std::lower_bound(cells.begin(), cells.end(), static_cast<Bits>(cellKey(x, below)));
                    const auto hi = std::upper_bound(lo, cells.end(), static_cast<Bits>(cellKey(x, above)));
                    ranges.emplace_back(static_cast<Index>(lo - cells.begin()), static_cast<Index>(hi - cells.begin()));
                }
            } else {
                ranges.emplace_back(0u, static_cast<Index>(m));
            }
            for (const auto & [lo, hi] : ranges) {
                for (Index place = lo; place < hi; ++place) {
                    const Index j = cellOrder[place];
                    if (j > i && reaches(DeviceComplex(walk[j].real(), walk[j].imag()), centre, epsilon.at(f))) {
                        const Index a = find(i);
                        const Index b = find(j);
                        if (a != b) {
                            parent[std::max(a, b)] = std::min(a, b);
                        }
                    }
                }
            }
        }

        std::vector<Index> label(m);
        std::vector<Bits> rightmost(m, 0);
        for (Index i = 0; i < m; ++i) {
            label[i] = find(i);
            rightmost[label[i]] = std::max<Bits>(rightmost[label[i]], orderedBits(walk[i].real()));
        }
        std::vector<Index> roots;
        for (Index i = 0; i < m; ++i) {
            if (label[i] == i) {
                roots.push_back(i);
            }
        }
        std::stable_sort(roots.begin(), roots.end(), [&](Index a, Index b) { return ~rightmost[a] < ~rightmost[b]; });
        std::vector<Index> rankOf(m);
        for (Index r = 0; r < roots.size(); ++r) {
            rankOf[roots[r]] = r;
        }
        std::vector<Index> members(m);
        std::iota(members.begin(), members.end(), 0u);
        std::stable_sort(members.begin(), members.end(), [&](Index a, Index b) {
            return rankOf[label[a]] < rankOf[label[b]];
        });
        for (std::size_t t = 0; t < m; ++t) {
            out.parts.members.push_back(static_cast<std::int32_t>(members[t]));
            if (t == 0 || rankOf[label[members[t]]] != rankOf[label[members[t - 1]]]) {
                out.parts.starts.push_back(t);
            }
        }
    }
    return prepared;
}

}
