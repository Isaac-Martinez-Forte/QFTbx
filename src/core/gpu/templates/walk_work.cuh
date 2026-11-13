/**
 * @file
 * @brief What the preparation of the clouds and their walk share on the
 * card.
 *
 * Work holds the buffers of a batch of frequencies, every cloud of it one
 * after another, and Sorter the stable radix sorts and the scans over them.
 * prepareBatch() leaves in Work, for each frequency, the points in MATLAB's
 * order without repeats, their projection, the frame of the neighbour grid,
 * the cells in order with the point of each, and the parts: their points
 * part after part, where each starts and which parts each frequency has.
 * A PointSet is the grid of one cloud as the threads search it: eleven
 * columns of cells round the centre, each found by a binary search; unite()
 * joins two points in a union-find without locks that always hangs the
 * larger root under the smaller (Jaiganesh and Burtscher 2018, ECL-CC).
 * hostPrepared() is the same preparation of one cloud run on the processor
 * with the threads' own code, for the tests, and hostColumns() and
 * hostCells() its search and its grid.
 */

#ifndef QFTBX_GPU_WALK_WORK_CUH
#define QFTBX_GPU_WALK_WORK_CUH

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include <cub/device/device_radix_sort.cuh>
#include <cub/device/device_scan.cuh>
#include <cub/util_type.cuh>
#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/launch.cuh"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/walk_keys.cuh"
#include "src/core/templates/cloud_set.h"
#include "src/core/templates/hull_metric.h"
#include "src/core/templates/walk_cloud.h"

namespace qftbx::gpu {

using Bits = unsigned long long;
using Index = std::uint32_t;

inline constexpr int kThreads = 256;
inline constexpr unsigned int kFull = 0xffffffffu;
inline constexpr Index kNoPoint = 0xffffffffu;
inline constexpr std::int64_t kCellLimit = (std::int64_t{1} << kCellBits) - 1;
inline constexpr std::size_t kLargestBatch = std::size_t{1} << 30;

QFTBX_HD inline double valueOfBits(Bits ordered)
{
    const std::uint64_t bits = (ordered >> 63) != 0 ? ordered & ~(std::uint64_t{1} << 63) : ~ordered;
    double value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

inline int bitsFor(std::size_t count)
{
    int bits = 1;
    while (bits < 32 && (std::size_t{1} << bits) < count) {
        ++bits;
    }
    return bits;
}

inline unsigned int blocks(std::size_t threads)
{
    return std::max(1u, blocksFor(threads, kThreads));
}

inline double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

std::size_t batchFrequencies(std::size_t frequencies, std::size_t plants, std::size_t bytesPerPoint);

struct PointSet
{
    const DeviceComplex * walk;
    const Bits * cells;
    const Index * order;
    Index first;
    Index last;
    GridFrame frame;
};

__device__ inline Index lowerBound(const Bits * keys, Index first, Index last, Bits key)
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

__device__ inline Index upperBound(const Bits * keys, Index first, Index last, Bits key)
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

__device__ inline void columnOf(const PointSet & set, DeviceComplex centre, int lane, Index & lo, Index & hi)
{
    lo = 0;
    hi = 0;
    if (set.frame.indexed) {
        const std::int64_t x = cellCoordinate(centre.real(), set.frame.minRe, set.frame.side) - kCellReach + lane;
        const std::int64_t y = cellCoordinate(centre.imag(), set.frame.minIm, set.frame.side);
        if (lane < kCellColumns && x >= 0 && x <= kCellLimit) {
            const std::int64_t below = y - kCellReach < 0 ? 0 : y - kCellReach;
            const std::int64_t above = y + kCellReach > kCellLimit ? kCellLimit : y + kCellReach;
            lo = lowerBound(set.cells, set.first, set.last, static_cast<Bits>(cellKey(x, below)));
            hi = upperBound(set.cells, lo, set.last, static_cast<Bits>(cellKey(x, above)));
        }
    } else if (lane == 0) {
        lo = set.first;
        hi = set.last;
    }
}

__device__ inline Index candidateAt(Index c, Index exclusive, Index lo)
{
    Index place = 0;
    for (int k = 0; k < kCellColumns; ++k) {
        const Index from = __shfl_sync(kFull, exclusive, k);
        const Index first = __shfl_sync(kFull, lo, k);
        place = c >= from ? first + (c - from) : place;
    }
    return place;
}

__device__ inline Index warpExclusive(Index count, int lane, Index & total)
{
    Index inclusive = count;
    for (int d = 1; d < 32; d <<= 1) {
        const Index below = __shfl_up_sync(kFull, inclusive, d);
        inclusive += lane >= d ? below : 0u;
    }
    total = __shfl_sync(kFull, inclusive, 31);
    return inclusive - count;
}

__device__ inline Index representative(Index * parent, Index x)
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

__device__ inline void unite(Index * parent, Index a, Index b)
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
    static constexpr std::size_t kBytesPerPoint = 4 * sizeof(Bits) + 13 * sizeof(Index)
            + 2 * sizeof(DeviceComplex) + sizeof(double);

    Work(std::size_t points, std::size_t frequencies, bool nichols, const Stream & stream)
        : keysA(points, stream), keysB(points, stream), valuesA(points, stream), valuesB(points, stream),
          flags(points + 1, stream), places(points + 1, stream), cloudPoints(points, stream), walk(points, stream),
          frequencyOf(points, stream), parent(points, stream), label(points, stream), rankOf(points, stream),
          starts(points, stream), rightmost(points, stream), phases(nichols ? points : 0, stream),
          cells(points, stream), cellOrder(points, stream), members(points, stream),
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
    DeviceBuffer<Bits> cells;
    DeviceBuffer<Index> cellOrder;
    DeviceBuffer<Index> members;
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

struct PreparedBatch
{
    Index points = 0;
    Index parts = 0;
};

PreparedBatch prepareBatch(Work & work, const DeviceComplex * cloud, Index plants, Index frequencies,
                           HullMetric metric, double dbPerDegree, cudaStream_t stream);

struct HostPrepared
{
    PreparedCloud prepared;
    GridFrame frame;
    std::vector<Bits> cells;
    std::vector<Index> cellOrder;
    bool shift = false;
    double cut = 0.0;
};

HostPrepared hostPrepared(const ComplexCloud & cloud, double epsilon, HullMetric metric, double dbPerDegree);

std::vector<std::pair<Index, Index>> hostColumns(const GridFrame & frame, const std::vector<Bits> & cells,
                                                 DeviceComplex centre);

void hostCells(const GridFrame & frame, const ComplexCloud & walk, std::vector<Bits> & cells,
               std::vector<Index> & cellOrder);

}

#endif
