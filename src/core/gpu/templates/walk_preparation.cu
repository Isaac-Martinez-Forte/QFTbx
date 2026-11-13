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
#include "src/core/gpu/templates/walk_work.cuh"

namespace qftbx::gpu {

namespace {

constexpr int kBoundsThreads = 256;
constexpr int kPartWarps = 4;
constexpr int kPartBlock = 32 * kPartWarps;

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

        const PointSet set{walk, cells, cellOrder, start, end, frame};
        Index lo = 0;
        Index hi = 0;
        columnOf(set, centre, lane, lo, hi);
        Index total = 0;
        const Index exclusive = warpExclusive(hi - lo, lane, total);

        for (Index base = 0; base < total; base += 32) {
            const Index c = base + static_cast<Index>(lane);
            const Index place = candidateAt(c, exclusive, lo);
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

Index * narrowed(DeviceBuffer<Bits> & keys)
{
    return reinterpret_cast<Index *>(keys.data());
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

Index splitIntoParts(Work & work, Index m, Index plants, Index frequencies, cudaStream_t stream)
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
    check(cudaMemcpyAsync(work.cellOrder.data(), order, m * sizeof(Index), cudaMemcpyDeviceToDevice, stream),
          "cudaMemcpyAsync of the order of the cells");
    sortedCells<<<blocks(m), kThreads, 0, stream>>>(work.walk.data(), work.frequencyOf.data(), work.frames.data(),
                                                    order, m, work.cells.data());
    checkLaunch("the kernel of the sorted cells");

    initialParents<<<blocks(m), kThreads, 0, stream>>>(m, work.parent.data());
    checkLaunch("the kernel of the first parents");
    const unsigned int partBlocks = residentBlocks(uniteParts, kPartBlock, (m + kPartWarps - 1) / kPartWarps);
    uniteParts<<<partBlocks, kPartBlock, 0, stream>>>(work.walk.data(), work.frequencyOf.data(), work.offsets.data(),
                                                      work.frames.data(), work.epsilon.data(), work.cells.data(),
                                                      work.cellOrder.data(), m, work.parent.data());
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
    Index * members = work.valuesA.data();
    spare = work.valuesB.data();
    memberKeys<<<blocks(m), kThreads, 0, stream>>>(work.label.data(), work.rankOf.data(), m, memberRanks, members);
    checkLaunch("the kernel of the members");
    work.sorter.sort(memberRanks, narrowed(work.keysB), members, spare, m, bitsFor(roots));
    partStarts<<<blocks(m), kThreads, 0, stream>>>(memberRanks, m, work.starts.data());
    checkLaunch("the kernel of the starts of the parts");
    check(cudaMemcpyAsync(work.members.data(), members, m * sizeof(Index), cudaMemcpyDeviceToDevice, stream),
          "cudaMemcpyAsync of the members");
    return roots;
}

}

std::size_t batchFrequencies(std::size_t frequencies, std::size_t plants, std::size_t bytesPerPoint)
{
    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    check(cudaMemGetInfo(&freeBytes, &totalBytes), "cudaMemGetInfo");
    const std::size_t perFrequency = plants * bytesPerPoint * 2;
    if (perFrequency > freeBytes / 2 || plants > kLargestBatch) {
        throw CudaError("the card's free memory cannot hold the contour of one frequency", 0);
    }
    return std::min<std::size_t>({frequencies, (freeBytes / 2) / perFrequency, kLargestBatch / plants, 65535});
}

Index sortedBatch(Work & work, const DeviceComplex * cloud, Index plants, Index frequencies, HullMetric metric,
                  double dbPerDegree, cudaStream_t stream)
{
    const Index points = sortedWithoutRepeats(work, cloud, frequencies * plants, plants, frequencies, stream);
    if (metric == HullMetric::Nichols) {
        projectOntoNichols(work, points, frequencies, dbPerDegree, stream);
    } else {
        check(cudaMemcpyAsync(work.walk.data(), work.cloudPoints.data(), points * sizeof(DeviceComplex),
                              cudaMemcpyDeviceToDevice, stream), "cudaMemcpyAsync of the walk");
    }
    return points;
}

PreparedBatch prepareBatch(Work & work, const DeviceComplex * cloud, Index plants, Index frequencies,
                           HullMetric metric, double dbPerDegree, cudaStream_t stream)
{
    PreparedBatch batch;
    batch.points = sortedBatch(work, cloud, plants, frequencies, metric, dbPerDegree, stream);
    batch.parts = splitIntoParts(work, batch.points, plants, frequencies, stream);
    return batch;
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

std::shared_ptr<const ResidentClouds> residentOf(std::shared_ptr<const DeviceClouds> clouds)
{
    return clouds;
}

std::shared_ptr<const DeviceClouds> deviceCloudsOf(std::shared_ptr<const ResidentClouds> clouds)
{
    return std::dynamic_pointer_cast<const DeviceClouds>(std::move(clouds));
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

    const std::size_t batch = batchFrequencies(frequencyCount, plants, Work::kBytesPerPoint);
    Work work(batch * plants, batch, nichols, stream);

    for (std::size_t first = 0; first < frequencyCount; first += batch) {
        const Index count = static_cast<Index>(std::min(batch, frequencyCount - first));
        const DeviceComplex * cloud = clouds.values() + first * plants;

        const auto computing = std::chrono::steady_clock::now();
        check(cudaMemcpyAsync(work.epsilon.data(), epsilon.data() + first, count * sizeof(double),
                              cudaMemcpyHostToDevice, stream.get()), "cudaMemcpyAsync of the epsilons");
        const PreparedBatch prepared = prepareBatch(work, cloud, static_cast<Index>(plants), count, metric,
                                                    dbPerDegree, stream.get());
        const Index m = prepared.points;
        const Index roots = prepared.parts;
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
        download(memberList.data(), work.members.data(), m * sizeof(Index), "cudaMemcpyAsync of the members");
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

std::vector<std::pair<Index, Index>> hostColumns(const GridFrame & frame, const std::vector<Bits> & cells,
                                                 DeviceComplex centre)
{
    std::vector<std::pair<Index, Index>> ranges;
    if (!frame.indexed) {
        ranges.emplace_back(0u, static_cast<Index>(cells.size()));
        return ranges;
    }
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
    return ranges;
}

void hostCells(const GridFrame & frame, const ComplexCloud & walk, std::vector<Bits> & cells,
               std::vector<Index> & cellOrder)
{
    const std::size_t m = walk.size();
    std::vector<Bits> cellOf(m);
    for (std::size_t i = 0; i < m; ++i) {
        cellOf[i] = cellKeyOf(frame, DeviceComplex(walk[i].real(), walk[i].imag()));
    }
    cellOrder.resize(m);
    std::iota(cellOrder.begin(), cellOrder.end(), 0u);
    std::stable_sort(cellOrder.begin(), cellOrder.end(), [&](Index a, Index b) { return cellOf[a] < cellOf[b]; });
    cells.resize(m);
    for (std::size_t t = 0; t < m; ++t) {
        cells[t] = cellOf[cellOrder[t]];
    }
}

HostSorted hostSorted(const ComplexCloud & cloud, HullMetric metric, double dbPerDegree)
{
    HostSorted host;
    SortedCloud & out = host.sorted;
    const std::size_t n = cloud.size();
    if (n == 0) {
        return host;
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
            out.points.push_back(cloud[order[t]]);
        }
    }
    const std::size_t m = out.points.size();

    if (metric == HullMetric::Nichols) {
        std::vector<double> phases(m);
        for (std::size_t i = 0; i < m; ++i) {
            phases[i] = phaseOf(DeviceComplex(out.points[i].real(), out.points[i].imag()));
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
        host.shift = m >= 2 && phases[at] - phases[at - 1] > wrap;
        host.cut = host.shift ? phases[at - 1] : 0.0;
        for (const std::complex<double> & z : out.points) {
            const DeviceComplex w = nicholsPoint(DeviceComplex(z.real(), z.imag()), dbPerDegree, host.shift, host.cut);
            out.walk.emplace_back(w.real(), w.imag());
        }
    } else {
        out.walk = out.points;
    }
    return host;
}

HostPrepared hostPrepared(const ComplexCloud & cloud, double epsilon, HullMetric metric, double dbPerDegree)
{
    HostPrepared host;
    PreparedCloud & out = host.prepared;
    if (cloud.empty()) {
        return host;
    }
    HostSorted sorted = hostSorted(cloud, metric, dbPerDegree);
    out.sorted = std::move(sorted.sorted);
    host.shift = sorted.shift;
    host.cut = sorted.cut;
    const std::size_t m = out.sorted.points.size();
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
    host.frame = gridFrame(minRe, maxRe, minIm, maxIm, epsilon);
    hostCells(host.frame, walk, host.cells, host.cellOrder);

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
        for (const auto & [lo, hi] : hostColumns(host.frame, host.cells, centre)) {
            for (Index place = lo; place < hi; ++place) {
                const Index j = host.cellOrder[place];
                if (j > i && reaches(DeviceComplex(walk[j].real(), walk[j].imag()), centre, epsilon)) {
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
    return host;
}

std::vector<PreparedCloud> preparedOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                               HullMetric metric, double dbPerDegree)
{
    std::vector<PreparedCloud> prepared;
    for (std::size_t f = 0; f < clouds.size(); ++f) {
        prepared.push_back(hostPrepared(clouds[f], epsilon.at(f), metric, dbPerDegree).prepared);
    }
    return prepared;
}

}
