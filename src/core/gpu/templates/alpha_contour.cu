/**
 * @file
 * @brief The kernels of the alpha-shape and their run.
 *
 * A warp takes a point and its candidates after it within epsilon one by
 * one; for each, its threads look for a blocker of either disc among the
 * point's neighbours, 32 at a time, and stop as soon as both are blocked.
 * A first pass only counts the candidates, so that the second writes the
 * edges in place without testing them twice. The edges are sorted, the
 * neighbours of every point ordered by angle and then by index, and the
 * components joined by the union-find of the preparation. A thread walks
 * the faces of a component in the order alphaShape() does, the same code
 * on both processors, and keeps the one of largest area: sequential by
 * nature, but one thread per component. The faces of every component and
 * the isolated points come back to the processor. A warp asks every 32
 * points, and a thread every 1024 steps of a face, whether the computation
 * was cancelled, and stops if it was.
 */

#include "src/core/gpu/templates/alpha_contour.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <cuda_runtime.h>

#include "src/core/gpu/common/cancel_flag.cuh"

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/alpha_discs.cuh"
#include "src/core/gpu/templates/device_clouds.cuh"
#include "src/core/gpu/templates/walk_keys.cuh"
#include "src/core/gpu/templates/walk_work.cuh"
#include "src/core/templates/alpha_shape.h"

namespace qftbx::gpu {

namespace {

constexpr int kEdgeWarps = 4;
constexpr int kEdgeBlock = 32 * kEdgeWarps;
constexpr Bits kLowHalf = 0xffffffffull;
constexpr Index kPollPoints = 32;
constexpr Index kPollSteps = 1024;
constexpr std::size_t kAlphaBytesPerPoint = 12 * sizeof(Index) + 4 * sizeof(Bits) + sizeof(double)
        + 3 * sizeof(DeviceComplex);

struct FaceGraph
{
    const Bits * edges;
    Index edgeCount;
    const Index * adjOffsets;
    const Index * adjacency;
    const DeviceComplex * walk;
    unsigned char * used;
    Index * scratch;
};

QFTBX_HD inline Index halfEdge(const FaceGraph & g, Index u, Index w)
{
    const Bits key = (static_cast<Bits>(u < w ? u : w) << 32) | static_cast<Bits>(u < w ? w : u);
    Index first = 0;
    Index last = g.edgeCount;
    while (first < last) {
        const Index middle = first + (last - first) / 2;
        if (g.edges[middle] < key) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return 2 * first + (u > w ? 1u : 0u);
}

QFTBX_HD inline void largestFace(const FaceGraph & g, const Index * componentEdges, Index from, Index to,
                                 Index & bestAt, Index & bestLength, int * cancel)
{
    const Index base = 2 * from;
    Index written = 0;
    Index steps = 0;
    double bestArea = 0.0;
    bestAt = base;
    bestLength = 0;
    for (Index q = from; q < to; ++q) {
        const Index e = componentEdges[q];
        const Index i = static_cast<Index>(g.edges[e] >> 32);
        const Index j = static_cast<Index>(g.edges[e] & kLowHalf);
        for (int direction = 0; direction < 2; ++direction) {
            Index u = direction == 0 ? i : j;
            Index w = direction == 0 ? j : i;
            Index h = 2 * e + static_cast<Index>(direction);
            if (g.used[h] != 0) {
                continue;
            }
            const Index begin = written;
            while (g.used[h] == 0) {
                if (++steps % kPollSteps == 0 && cancelAsked(cancel)) {
                    bestLength = 0;
                    return;
                }
                g.used[h] = 1;
                g.scratch[base + written] = u;
                ++written;
                const Index first = g.adjOffsets[w];
                const Index degree = g.adjOffsets[w + 1] - first;
                Index position = 0;
                while (g.adjacency[first + position] != u) {
                    ++position;
                }
                const Index next = g.adjacency[first + (position + degree - 1) % degree];
                u = w;
                w = next;
                h = halfEdge(g, u, w);
            }
            const Index length = written - begin;
            double area = 0.0;
            for (Index k = 0; k < length; ++k) {
                const DeviceComplex a = g.walk[g.scratch[base + begin + k]];
                const DeviceComplex b = g.walk[g.scratch[base + begin + (k + 1) % length]];
                area += a.real() * b.imag() - b.real() * a.imag();
            }
            area = ::fabs(area) / 2.0;
            if (bestLength == 0 || area > bestArea) {
                bestArea = area;
                bestAt = base + begin;
                bestLength = length;
            }
        }
    }
}

struct BatchView
{
    const DeviceComplex * walk;
    const Index * frequencyOf;
    const Index * offsets;
    const GridFrame * frames;
    const double * epsilon;
    const Bits * cells;
    const Index * cellOrder;
    Index points;
    int * cancel;
};

__device__ PointSet setOf(const BatchView & v, Index f)
{
    return {v.walk, v.cells, v.cellOrder, v.offsets[f], v.offsets[f + 1], v.frames[f]};
}

__device__ Index warpSum(Index value)
{
    for (int d = 16; d > 0; d >>= 1) {
        value += __shfl_down_sync(kFull, value, d);
    }
    return __shfl_sync(kFull, value, 0);
}

__global__ void __launch_bounds__(kEdgeBlock) pairBounds(BatchView v, Index * bound)
{
    const int lane = static_cast<int>(threadIdx.x % 32);
    const Index warps = gridDim.x * kEdgeWarps;
    for (Index i = blockIdx.x * kEdgeWarps + threadIdx.x / 32; i < v.points; i += warps) {
        const Index f = v.frequencyOf[i];
        const PointSet set = setOf(v, f);
        const double epsilon = v.epsilon[f];
        const DeviceComplex centre = v.walk[i];
        Index lo = 0;
        Index hi = 0;
        columnOf(set, centre, lane, lo, hi);
        Index total = 0;
        const Index exclusive = warpExclusive(hi - lo, lane, total);
        Index count = 0;
        for (Index base = 0; base < total; base += 32) {
            const Index c = base + static_cast<Index>(lane);
            const Index place = candidateAt(c, exclusive, lo);
            if (c < total) {
                const Index j = v.cellOrder[place];
                count += j > i && reaches(v.walk[j], centre, epsilon) ? 1u : 0u;
            }
        }
        count = warpSum(count);
        if (lane == 0) {
            bound[i] = count;
        }
    }
}

__global__ void __launch_bounds__(kEdgeBlock) boundaryEdges(BatchView v, const Index * slots, Bits * keys,
                                                            Index * accepted)
{
    const int lane = static_cast<int>(threadIdx.x % 32);
    const Index warps = gridDim.x * kEdgeWarps;
    Index handled = 0;
    for (Index i = blockIdx.x * kEdgeWarps + threadIdx.x / 32; i < v.points; i += warps) {
        if (++handled % kPollPoints == 0 && warpCancelRequested(v.cancel)) {
            return;
        }
        const Index f = v.frequencyOf[i];
        const PointSet set = setOf(v, f);
        const double epsilon = v.epsilon[f];
        const DeviceComplex centre = v.walk[i];
        Index lo = 0;
        Index hi = 0;
        columnOf(set, centre, lane, lo, hi);
        Index total = 0;
        const Index exclusive = warpExclusive(hi - lo, lane, total);
        Index written = 0;
        for (Index c = 0; c < total; ++c) {
            const Index j = v.cellOrder[candidateAt(c, exclusive, lo)];
            if (!(j > i && reaches(v.walk[j], centre, epsilon))) {
                continue;
            }
            const Discs discs = discsOf(centre, v.walk[j], epsilon);
            bool edge = false;
            if (discs.chord) {
                bool blockedFirst = false;
                bool blockedSecond = false;
                for (Index base = 0; base < total && !(blockedFirst && blockedSecond); base += 32) {
                    const Index d = base + static_cast<Index>(lane);
                    const Index place = candidateAt(d, exclusive, lo);
                    bool first = false;
                    bool second = false;
                    if (d < total) {
                        const Index k = v.cellOrder[place];
                        if (k != i && k != j && reaches(v.walk[k], centre, epsilon)) {
                            first = blocks(v.walk[k], discs.first, discs.blocking);
                            second = blocks(v.walk[k], discs.second, discs.blocking);
                        }
                    }
                    blockedFirst = __any_sync(kFull, blockedFirst || first);
                    blockedSecond = __any_sync(kFull, blockedSecond || second);
                }
                edge = !blockedFirst || !blockedSecond;
            }
            if (edge) {
                if (lane == 0) {
                    keys[slots[i] + written] = (static_cast<Bits>(i) << 32) | static_cast<Bits>(j);
                }
                ++written;
            }
        }
        if (lane == 0) {
            accepted[i] = written;
        }
    }
}

__global__ void compactEdges(const Bits * keys, const Index * slots, const Index * accepted, const Index * places,
                             Index m, Bits * edges)
{
    const Index i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < m) {
        for (Index k = 0; k < accepted[i]; ++k) {
            edges[places[i] + k] = keys[slots[i] + k];
        }
    }
}

__global__ void edgeIndices(Index e, Index * values)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e) {
        values[t] = t;
    }
}

__global__ void edgeDegrees(const Bits * edges, Index e, Index * degree)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e) {
        atomicAdd(degree + static_cast<Index>(edges[t] >> 32), 1u);
        atomicAdd(degree + static_cast<Index>(edges[t] & kLowHalf), 1u);
    }
}

__global__ void fillAdjacency(const Bits * edges, Index e, const Index * adjOffsets, Index * cursor,
                              Index * adjacency)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e) {
        const Index i = static_cast<Index>(edges[t] >> 32);
        const Index j = static_cast<Index>(edges[t] & kLowHalf);
        adjacency[adjOffsets[i] + atomicAdd(cursor + i, 1u)] = j;
        adjacency[adjOffsets[j] + atomicAdd(cursor + j, 1u)] = i;
    }
}

__global__ void orderNeighbours(const DeviceComplex * walk, const Index * adjOffsets, Index m, Index * adjacency,
                                double * angles)
{
    const Index v = blockIdx.x * blockDim.x + threadIdx.x;
    if (v >= m) {
        return;
    }
    const Index from = adjOffsets[v];
    const Index to = adjOffsets[v + 1];
    for (Index q = from; q < to; ++q) {
        angles[q] = angleTowards(walk[v], walk[adjacency[q]]);
    }
    for (Index q = from + 1; q < to; ++q) {
        const double angle = angles[q];
        const Index neighbour = adjacency[q];
        Index p = q;
        while (p > from && (angles[p - 1] > angle || (angles[p - 1] == angle && adjacency[p - 1] > neighbour))) {
            angles[p] = angles[p - 1];
            adjacency[p] = adjacency[p - 1];
            --p;
        }
        angles[p] = angle;
        adjacency[p] = neighbour;
    }
}

__global__ void firstParents(Index m, Index * parent)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < m) {
        parent[t] = t;
    }
}

__global__ void uniteEdges(const Bits * edges, Index e, Index * parent)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e) {
        unite(parent, static_cast<Index>(edges[t] >> 32), static_cast<Index>(edges[t] & kLowHalf));
    }
}

__global__ void componentKeys(const Bits * edges, Index * parent, Index e, Index * keys)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e) {
        keys[t] = representative(parent, static_cast<Index>(edges[t] >> 32));
    }
}

__global__ void componentStarts(const Index * keys, Index e, Index * flags)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e) {
        flags[t] = t == 0 || keys[t] != keys[t - 1] ? 1u : 0u;
    } else if (t == e) {
        flags[t] = 0u;
    }
}

__global__ void componentRanges(const Index * keys, const Index * flags, const Index * places,
                                const Index * frequencyOf, Index e, Index * start, Index * frequency)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < e && flags[t] != 0) {
        start[places[t]] = t;
        frequency[places[t]] = frequencyOf[keys[t]];
    }
    if (t == 0) {
        start[places[e]] = e;
    }
}

__global__ void largestFaces(FaceGraph g, const Index * componentEdges, const Index * start, Index components,
                             Index * faceAt, Index * faceLength, int * cancel)
{
    const Index c = blockIdx.x * blockDim.x + threadIdx.x;
    if (c < components) {
        Index at = 0;
        Index length = 0;
        largestFace(g, componentEdges, start[c], start[c + 1], at, length, cancel);
        faceAt[c] = at;
        faceLength[c] = length;
    }
    if (c == components) {
        faceLength[c] = 0;
    }
}

__global__ void isolatedFlags(const Index * partStarts, Index parts, Index m, Index * flags)
{
    const Index r = blockIdx.x * blockDim.x + threadIdx.x;
    if (r < parts) {
        const Index to = r + 1 < parts ? partStarts[r + 1] : m;
        flags[r] = to - partStarts[r] == 1 ? 1u : 0u;
    } else if (r == parts) {
        flags[r] = 0u;
    }
}

struct LoopEntry
{
    Index index;
    DeviceComplex walk;
    DeviceComplex point;
};

__global__ void gatherFaces(const Index * scratch, const Index * faceAt, const Index * places, Index components,
                            const DeviceComplex * walk, const DeviceComplex * points, LoopEntry * out)
{
    const Index c = blockIdx.x * blockDim.x + threadIdx.x;
    if (c < components) {
        const Index count = places[c + 1] - places[c];
        for (Index k = 0; k < count; ++k) {
            const Index index = scratch[faceAt[c] + k];
            out[places[c] + k] = {index, walk[index], points[index]};
        }
    }
}

__global__ void gatherIsolated(const Index * partStarts, const Index * members, const Index * flags,
                               const Index * places, Index parts, const DeviceComplex * walk,
                               const DeviceComplex * points, LoopEntry * out)
{
    const Index r = blockIdx.x * blockDim.x + threadIdx.x;
    if (r < parts && flags[r] != 0) {
        const Index index = members[partStarts[r]];
        out[places[r]] = {index, walk[index], points[index]};
    }
}

WalkedContour finished(const std::vector<std::vector<Index>> & faces, const std::vector<Index> & isolated,
                       const std::vector<std::pair<std::complex<double>, std::complex<double>>> & byIndex,
                       Index first)
{
    ComplexCloud walk(byIndex.size());
    ComplexCloud points(byIndex.size());
    for (std::size_t k = 0; k < byIndex.size(); ++k) {
        walk[k] = byIndex[k].first;
        points[k] = byIndex[k].second;
    }
    std::vector<std::vector<std::int32_t>> candidates;
    for (const std::vector<Index> & face : faces) {
        std::vector<std::int32_t> loop;
        for (const Index index : face) {
            loop.push_back(static_cast<std::int32_t>(index - first));
        }
        candidates.push_back(std::move(loop));
    }
    std::vector<std::int32_t> alone;
    for (const Index index : isolated) {
        alone.push_back(static_cast<std::int32_t>(index - first));
    }
    std::sort(alone.begin(), alone.end());
    WalkedContour out;
    out.contour = loopsContour(points, finishedLoops(walk, std::move(candidates), alone), &out.starts);
    return out;
}

std::complex<double> onHost(DeviceComplex z)
{
    return {z.real(), z.imag()};
}

DeviceComplex onCard(const std::complex<double> & z)
{
    return DeviceComplex(z.real(), z.imag());
}

}

AlphaOnCard alphaOnCard(const DeviceClouds & clouds, const std::vector<double> & epsilon, HullMetric metric,
                        double dbPerDegree, const CancellationToken * cancellation)
{
    AlphaOnCard result;
    const std::size_t frequencyCount = clouds.frequencies();
    const std::size_t plants = clouds.plants();
    result.contours.assign(frequencyCount, {});
    if (frequencyCount == 0 || plants == 0) {
        return result;
    }
    if (epsilon.size() < frequencyCount) {
        throw CudaError("the clouds were given " + std::to_string(epsilon.size()) + " epsilons for "
                        + std::to_string(frequencyCount) + " frequencies", 0);
    }

    useProbedDevice();
    const Stream & stream = clouds.stream();
    const cudaStream_t s = stream.get();
    const bool nichols = metric == HullMetric::Nichols;
    const std::size_t batch = batchFrequencies(frequencyCount, plants, Work::kBytesPerPoint + kAlphaBytesPerPoint);
    const std::size_t points = batch * plants;
    Work work(points, batch, nichols, stream);
    DeviceBuffer<Index> bound(points + 1, stream);
    DeviceBuffer<Index> slots(points + 1, stream);
    DeviceBuffer<Index> accepted(points + 1, stream);
    DeviceBuffer<Index> places(points + 1, stream);
    DeviceBuffer<Index> degree(points + 1, stream);
    DeviceBuffer<Index> adjOffsets(points + 1, stream);
    DeviceBuffer<Index> cursor(points, stream);
    DeviceBuffer<Index> parent(points, stream);
    const auto edgeBlocks = [&](auto kernel, std::size_t warpsNeeded) {
        int perMultiprocessor = 0;
        check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perMultiprocessor, kernel, kEdgeBlock, 0),
              "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
        const std::size_t resident = static_cast<std::size_t>(std::max(perMultiprocessor, 1))
                * static_cast<std::size_t>(std::max(probe().multiprocessors, 1));
        const std::size_t needed = (warpsNeeded + kEdgeWarps - 1) / kEdgeWarps;
        return static_cast<unsigned int>(std::max<std::size_t>(1, std::min(resident, needed)));
    };
    CancelFlag flag;
    const auto readBack = [&](Index * to, const Index * from) {
        check(cudaMemcpyAsync(to, from, sizeof(Index), cudaMemcpyDeviceToHost, s), "cudaMemcpyAsync of a count");
        stream.synchronize();
    };
    const auto readBackUnlessCancelled = [&](Index * to, const Index * from) {
        check(cudaMemcpyAsync(to, from, sizeof(Index), cudaMemcpyDeviceToHost, s), "cudaMemcpyAsync of a count");
        return stream.finish(cancellation, &flag);
    };

    for (std::size_t first = 0; first < frequencyCount; first += batch) {
        const Index count = static_cast<Index>(std::min(batch, frequencyCount - first));
        const DeviceComplex * cloud = clouds.values() + first * plants;

        const auto computing = std::chrono::steady_clock::now();
        check(cudaMemcpyAsync(work.epsilon.data(), epsilon.data() + first, count * sizeof(double),
                              cudaMemcpyHostToDevice, s), "cudaMemcpyAsync of the epsilons");
        const PreparedBatch prepared = prepareBatch(work, cloud, static_cast<Index>(plants), count, metric,
                                                    dbPerDegree, s);
        const Index m = prepared.points;
        const Index parts = prepared.parts;
        const BatchView view{work.walk.data(), work.frequencyOf.data(), work.offsets.data(), work.frames.data(),
                             work.epsilon.data(), work.cells.data(), work.cellOrder.data(), m, flag.device()};

        bound.fill(0);
        pairBounds<<<edgeBlocks(pairBounds, m), kEdgeBlock, 0, s>>>(view, bound.data());
        checkLaunch("the kernel of the pairs");
        work.sorter.scan(bound.data(), slots.data(), m + 1);
        Index pairs = 0;
        readBack(&pairs, slots.data() + m);
        DeviceBuffer<Bits> keys(std::max<Index>(pairs, 1), stream);
        accepted.fill(0);
        boundaryEdges<<<edgeBlocks(boundaryEdges, m), kEdgeBlock, 0, s>>>(view, slots.data(), keys.data(),
                                                                          accepted.data());
        checkLaunch("the kernel of the boundary edges");
        work.sorter.scan(accepted.data(), places.data(), m + 1);
        Index edgeCount = 0;
        if (!readBackUnlessCancelled(&edgeCount, places.data() + m)) {
            result.cancelled = true;
            return result;
        }

        const std::size_t edgeRoom = std::max<Index>(edgeCount, 1);
        DeviceBuffer<Bits> edgesA(edgeRoom, stream);
        DeviceBuffer<Bits> edgesB(edgeRoom, stream);
        DeviceBuffer<Index> valuesA(edgeRoom + 1, stream);
        DeviceBuffer<Index> valuesB(edgeRoom + 1, stream);
        DeviceBuffer<Index> componentA(edgeRoom + 1, stream);
        DeviceBuffer<Index> componentB(edgeRoom + 1, stream);
        DeviceBuffer<Index> adjacency(2 * edgeRoom, stream);
        DeviceBuffer<double> angles(2 * edgeRoom, stream);
        DeviceBuffer<unsigned char> used(2 * edgeRoom, stream);
        DeviceBuffer<Index> scratch(2 * edgeRoom, stream);
        Sorter sorter(edgeRoom + 1, stream);
        compactEdges<<<blocks(m), kThreads, 0, s>>>(keys.data(), slots.data(), accepted.data(), places.data(), m,
                                                    edgesA.data());
        checkLaunch("the kernel of the compaction of the edges");
        Bits * edges = edgesA.data();
        Index * values = valuesA.data();
        Index * spare = valuesB.data();
        edgeIndices<<<blocks(edgeCount), kThreads, 0, s>>>(edgeCount, values);
        checkLaunch("the kernel of the indices of the edges");
        sorter.sort(edges, edgesB.data(), values, spare, edgeCount, 64);

        degree.fill(0);
        cursor.fill(0);
        edgeDegrees<<<blocks(edgeCount), kThreads, 0, s>>>(edges, edgeCount, degree.data());
        checkLaunch("the kernel of the degrees");
        work.sorter.scan(degree.data(), adjOffsets.data(), m + 1);
        fillAdjacency<<<blocks(edgeCount), kThreads, 0, s>>>(edges, edgeCount, adjOffsets.data(), cursor.data(),
                                                             adjacency.data());
        checkLaunch("the kernel of the adjacency");
        orderNeighbours<<<blocks(m), kThreads, 0, s>>>(work.walk.data(), adjOffsets.data(), m, adjacency.data(),
                                                       angles.data());
        checkLaunch("the kernel of the order of the neighbours");

        firstParents<<<blocks(m), kThreads, 0, s>>>(m, parent.data());
        checkLaunch("the kernel of the first parents");
        uniteEdges<<<blocks(edgeCount), kThreads, 0, s>>>(edges, edgeCount, parent.data());
        checkLaunch("the kernel of the components of the edges");
        Index * componentKeysOf = componentA.data();
        Index * componentEdges = valuesA.data();
        Index * componentSpare = valuesB.data();
        componentKeys<<<blocks(edgeCount), kThreads, 0, s>>>(edges, parent.data(), edgeCount, componentKeysOf);
        checkLaunch("the kernel of the keys of the components");
        edgeIndices<<<blocks(edgeCount), kThreads, 0, s>>>(edgeCount, componentEdges);
        checkLaunch("the kernel of the indices of the edges");
        sorter.sort(componentKeysOf, componentB.data(), componentEdges, componentSpare, edgeCount, bitsFor(m));
        Index * flags = componentKeysOf == componentA.data() ? componentB.data() : componentA.data();
        componentStarts<<<blocks(edgeCount + 1), kThreads, 0, s>>>(componentKeysOf, edgeCount, flags);
        checkLaunch("the kernel of the starts of the components");
        sorter.scan(flags, componentSpare, edgeCount + 1);
        Index components = 0;
        readBack(&components, componentSpare + edgeCount);
        DeviceBuffer<Index> start(components + 1, stream);
        DeviceBuffer<Index> componentFrequency(std::max<Index>(components, 1), stream);
        DeviceBuffer<Index> faceAt(std::max<Index>(components, 1), stream);
        DeviceBuffer<Index> faceLength(components + 1, stream);
        DeviceBuffer<Index> facePlaces(components + 1, stream);
        componentRanges<<<blocks(edgeCount + 1), kThreads, 0, s>>>(componentKeysOf, flags, componentSpare,
                                                                   work.frequencyOf.data(), edgeCount, start.data(),
                                                                   componentFrequency.data());
        checkLaunch("the kernel of the ranges of the components");

        used.fill(0);
        const FaceGraph graph{edges, edgeCount, adjOffsets.data(), adjacency.data(), work.walk.data(), used.data(),
                              scratch.data()};
        largestFaces<<<blocks(components + 1), kThreads, 0, s>>>(graph, componentEdges, start.data(), components,
                                                                 faceAt.data(), faceLength.data(), flag.device());
        checkLaunch("the kernel of the faces");
        sorter.scan(faceLength.data(), facePlaces.data(), components + 1);
        Index faceEntries = 0;
        if (!readBackUnlessCancelled(&faceEntries, facePlaces.data() + components)) {
            result.cancelled = true;
            return result;
        }

        DeviceBuffer<Index> isolatedFlag(parts + 1, stream);
        DeviceBuffer<Index> isolatedPlaces(parts + 1, stream);
        isolatedFlags<<<blocks(parts + 1), kThreads, 0, s>>>(work.starts.data(), parts, m, isolatedFlag.data());
        checkLaunch("the kernel of the isolated points");
        work.sorter.scan(isolatedFlag.data(), isolatedPlaces.data(), parts + 1);
        Index isolatedCount = 0;
        readBack(&isolatedCount, isolatedPlaces.data() + parts);

        DeviceBuffer<LoopEntry> faceOut(std::max<Index>(faceEntries, 1), stream);
        DeviceBuffer<LoopEntry> isolatedOut(std::max<Index>(isolatedCount, 1), stream);
        gatherFaces<<<blocks(components), kThreads, 0, s>>>(scratch.data(), faceAt.data(), facePlaces.data(),
                                                            components, work.walk.data(), work.cloudPoints.data(),
                                                            faceOut.data());
        checkLaunch("the kernel of the gathering of the faces");
        gatherIsolated<<<blocks(parts), kThreads, 0, s>>>(work.starts.data(), work.members.data(),
                                                          isolatedFlag.data(), isolatedPlaces.data(), parts,
                                                          work.walk.data(), work.cloudPoints.data(),
                                                          isolatedOut.data());
        checkLaunch("the kernel of the gathering of the isolated points");
        stream.synchronize();
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        std::vector<Index> offsets(count + 1);
        std::vector<Index> frequencyOfComponent(components);
        std::vector<Index> placesOfFaces(components + 1);
        std::vector<LoopEntry> faceEntriesOnHost(faceEntries);
        std::vector<LoopEntry> isolatedOnHost(isolatedCount);
        const auto download = [&](void * to, const void * from, std::size_t bytes, const char * what) {
            if (bytes != 0) {
                check(cudaMemcpyAsync(to, from, bytes, cudaMemcpyDeviceToHost, s), what);
            }
        };
        download(offsets.data(), work.offsets.data(), (count + 1) * sizeof(Index), "cudaMemcpyAsync of the offsets");
        download(frequencyOfComponent.data(), componentFrequency.data(), components * sizeof(Index),
                 "cudaMemcpyAsync of the frequencies of the components");
        download(placesOfFaces.data(), facePlaces.data(), (components + 1) * sizeof(Index),
                 "cudaMemcpyAsync of the places of the faces");
        download(faceEntriesOnHost.data(), faceOut.data(), faceEntries * sizeof(LoopEntry),
                 "cudaMemcpyAsync of the faces");
        download(isolatedOnHost.data(), isolatedOut.data(), isolatedCount * sizeof(LoopEntry),
                 "cudaMemcpyAsync of the isolated points");
        stream.synchronize();

        std::vector<std::vector<std::vector<Index>>> faces(count);
        std::vector<std::vector<Index>> isolated(count);
        std::vector<std::vector<std::pair<std::complex<double>, std::complex<double>>>> byIndex(count);
        for (Index f = 0; f < count; ++f) {
            byIndex[f].resize(offsets[f + 1] - offsets[f]);
        }
        const auto frequencyOfIndex = [&](Index index) {
            return static_cast<Index>(std::upper_bound(offsets.begin(), offsets.end(), index) - offsets.begin() - 1);
        };
        for (Index c = 0; c < components; ++c) {
            const Index f = frequencyOfComponent[c];
            std::vector<Index> face;
            for (Index k = placesOfFaces[c]; k < placesOfFaces[c + 1]; ++k) {
                const LoopEntry & entry = faceEntriesOnHost[k];
                face.push_back(entry.index);
                byIndex[f][entry.index - offsets[f]] = {onHost(entry.walk), onHost(entry.point)};
            }
            faces[f].push_back(std::move(face));
        }
        for (const LoopEntry & entry : isolatedOnHost) {
            const Index f = frequencyOfIndex(entry.index);
            isolated[f].push_back(entry.index);
            byIndex[f][entry.index - offsets[f]] = {onHost(entry.walk), onHost(entry.point)};
        }
        for (Index f = 0; f < count; ++f) {
            if (epsilon[first + f] > 0.0) {
                result.contours[first + f] = finished(faces[f], isolated[f], byIndex[f], offsets[f]);
            }
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }
    return result;
}

std::vector<WalkedContour> alphaOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                            HullMetric metric, double dbPerDegree)
{
    std::vector<WalkedContour> contours(clouds.size());
    for (std::size_t f = 0; f < clouds.size(); ++f) {
        const double reach = epsilon.at(f);
        if (clouds[f].empty() || !(reach > 0.0)) {
            continue;
        }
        const HostPrepared host = hostPrepared(clouds[f], reach, metric, dbPerDegree);
        const ComplexCloud & walk = host.prepared.sorted.walk;
        const Index m = static_cast<Index>(walk.size());
        std::vector<DeviceComplex> onCardWalk(m);
        for (Index i = 0; i < m; ++i) {
            onCardWalk[i] = onCard(walk[i]);
        }

        std::vector<Bits> edges;
        for (Index i = 0; i < m; ++i) {
            const DeviceComplex centre = onCardWalk[i];
            std::vector<Index> near;
            for (const auto & [lo, hi] : hostColumns(host.frame, host.cells, centre)) {
                for (Index place = lo; place < hi; ++place) {
                    const Index k = host.cellOrder[place];
                    if (k != i && reaches(onCardWalk[k], centre, reach)) {
                        near.push_back(k);
                    }
                }
            }
            for (const Index j : near) {
                if (j <= i) {
                    continue;
                }
                const Discs discs = discsOf(centre, onCardWalk[j], reach);
                if (!discs.chord) {
                    continue;
                }
                bool blockedFirst = false;
                bool blockedSecond = false;
                for (const Index k : near) {
                    if (k != j) {
                        blockedFirst = blockedFirst || blocks(onCardWalk[k], discs.first, discs.blocking);
                        blockedSecond = blockedSecond || blocks(onCardWalk[k], discs.second, discs.blocking);
                    }
                }
                if (!blockedFirst || !blockedSecond) {
                    edges.push_back((static_cast<Bits>(i) << 32) | static_cast<Bits>(j));
                }
            }
        }
        std::sort(edges.begin(), edges.end());
        const Index edgeCount = static_cast<Index>(edges.size());

        std::vector<Index> adjOffsets(m + 1, 0);
        for (const Bits edge : edges) {
            ++adjOffsets[static_cast<Index>(edge >> 32) + 1];
            ++adjOffsets[static_cast<Index>(edge & kLowHalf) + 1];
        }
        std::partial_sum(adjOffsets.begin(), adjOffsets.end(), adjOffsets.begin());
        std::vector<Index> adjacency(2 * edges.size());
        std::vector<Index> cursor(m, 0);
        for (const Bits edge : edges) {
            const Index i = static_cast<Index>(edge >> 32);
            const Index j = static_cast<Index>(edge & kLowHalf);
            adjacency[adjOffsets[i] + cursor[i]++] = j;
            adjacency[adjOffsets[j] + cursor[j]++] = i;
        }
        for (Index v = 0; v < m; ++v) {
            std::sort(adjacency.begin() + adjOffsets[v], adjacency.begin() + adjOffsets[v + 1], [&](Index x, Index y) {
                const double ax = angleTowards(onCardWalk[v], onCardWalk[x]);
                const double ay = angleTowards(onCardWalk[v], onCardWalk[y]);
                return ax != ay ? ax < ay : x < y;
            });
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
        for (const Bits edge : edges) {
            const Index a = find(static_cast<Index>(edge >> 32));
            const Index b = find(static_cast<Index>(edge & kLowHalf));
            if (a != b) {
                parent[std::max(a, b)] = std::min(a, b);
            }
        }
        std::vector<Index> componentEdges(edgeCount);
        std::iota(componentEdges.begin(), componentEdges.end(), 0u);
        std::stable_sort(componentEdges.begin(), componentEdges.end(), [&](Index a, Index b) {
            return find(static_cast<Index>(edges[a] >> 32)) < find(static_cast<Index>(edges[b] >> 32));
        });

        std::vector<unsigned char> used(2 * edges.size(), 0);
        std::vector<Index> scratch(2 * edges.size(), 0);
        const FaceGraph graph{edges.data(), edgeCount, adjOffsets.data(), adjacency.data(), onCardWalk.data(),
                              used.data(), scratch.data()};
        std::vector<std::vector<Index>> faces;
        for (Index from = 0; from < edgeCount;) {
            Index to = from + 1;
            const Index root = find(static_cast<Index>(edges[componentEdges[from]] >> 32));
            while (to < edgeCount && find(static_cast<Index>(edges[componentEdges[to]] >> 32)) == root) {
                ++to;
            }
            Index at = 0;
            Index length = 0;
            largestFace(graph, componentEdges.data(), from, to, at, length, nullptr);
            faces.emplace_back(scratch.begin() + at, scratch.begin() + at + length);
            from = to;
        }

        std::vector<Index> isolated;
        const CloudParts & parts = host.prepared.parts;
        for (std::size_t p = 0; p < parts.starts.size(); ++p) {
            const std::size_t end = p + 1 < parts.starts.size() ? parts.starts[p + 1] : parts.members.size();
            if (end - parts.starts[p] == 1) {
                isolated.push_back(static_cast<Index>(parts.members[parts.starts[p]]));
            }
        }
        std::vector<std::pair<std::complex<double>, std::complex<double>>> byIndex(m);
        for (Index i = 0; i < m; ++i) {
            byIndex[i] = {walk[i], host.prepared.sorted.points[i]};
        }
        contours[f] = finished(faces, isolated, byIndex, 0);
    }
    return contours;
}

}
