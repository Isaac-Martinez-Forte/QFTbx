/**
 * @file
 * @brief The kernels of the walk and their run.
 *
 * A warp walks a part. At every step its threads find the eleven columns
 * of cells round the last point, weigh a candidate each and reduce to the
 * next point, without leaving the card; the first thread writes the steps,
 * at most three a point, in the part's own stretch of one array, and marks
 * the points the relaxed walk has kept. The relaxed walk over the part
 * runs in the same warp. The one over the cloud as swept, for a cloud that
 * is a single part, is rare, so its frequencies are projected, gridded and
 * walked apart, after the rest. A last kernel gathers the points of every
 * part into one array, and the processor joins them into contours as the
 * template engine does.
 */

#include "src/core/gpu/templates/contour_walk.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#include <cuda_runtime.h>
#include <math_constants.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/device_clouds.cuh"
#include "src/core/gpu/templates/walk_keys.cuh"
#include "src/core/gpu/templates/walk_steps.cuh"
#include "src/core/gpu/templates/walk_work.cuh"

namespace qftbx::gpu {

namespace {

constexpr int kWalkWarps = 2;
constexpr int kWalkBlock = 32 * kWalkWarps;
constexpr std::size_t kWalkBytesPerPoint = 3 * sizeof(Index) + sizeof(unsigned char) + 6 * sizeof(Index);

enum PartKind : int { kEmpty = 0, kWalked = 1, kRelaxed = 2, kTruncated = 3, kPendingRaw = 4 };

enum class WalkEnd { Closed, Dead, Exceeded };

__device__ Choice shuffledDown(const Choice & choice, int d)
{
    Choice other;
    other.angle = __shfl_down_sync(kFull, choice.angle, d);
    other.distance = __shfl_down_sync(kFull, choice.distance, d);
    other.index = __shfl_down_sync(kFull, choice.index, d);
    other.found = __shfl_down_sync(kFull, static_cast<int>(choice.found), d) != 0;
    return other;
}

__device__ Choice fromFirstLane(const Choice & choice)
{
    Choice first;
    first.angle = __shfl_sync(kFull, choice.angle, 0);
    first.distance = __shfl_sync(kFull, choice.distance, 0);
    first.index = __shfl_sync(kFull, choice.index, 0);
    first.found = __shfl_sync(kFull, static_cast<int>(choice.found), 0) != 0;
    return first;
}

template <typename Weigh>
__device__ Choice warpChoice(const PointSet & set, DeviceComplex centre, int lane, Weigh weigh)
{
    Index lo = 0;
    Index hi = 0;
    columnOf(set, centre, lane, lo, hi);
    Index total = 0;
    const Index exclusive = warpExclusive(hi - lo, lane, total);

    Choice best;
    for (Index base = 0; base < total; base += 32) {
        const Index c = base + static_cast<Index>(lane);
        const Index place = candidateAt(c, exclusive, lo);
        if (c < total) {
            const Index j = set.order[place];
            const Choice mine = weigh(set.walk[j], j);
            if (better(mine, best)) {
                best = mine;
            }
        }
    }
    for (int d = 16; d > 0; d >>= 1) {
        const Choice other = shuffledDown(best, d);
        if (better(other, best)) {
            best = other;
        }
    }
    return fromFirstLane(best);
}

__device__ Index firstExtreme(const DeviceComplex * walk, const Index * list, Index first, Index last, bool imaginary,
                              int lane)
{
    double best = -CUDART_INF;
    Index at = kNoPoint;
    for (Index k = first + static_cast<Index>(lane); k < last; k += 32) {
        const Index i = list != nullptr ? list[k] : k;
        const double value = imaginary ? walk[i].imag() : walk[i].real();
        if (at == kNoPoint || value > best) {
            best = value;
            at = i;
        }
    }
    for (int d = 16; d > 0; d >>= 1) {
        const double otherValue = __shfl_down_sync(kFull, best, d);
        const Index otherAt = __shfl_down_sync(kFull, at, d);
        const bool takes = otherAt != kNoPoint
                && (at == kNoPoint || otherValue > best || (otherValue == best && otherAt < at));
        best = takes ? otherValue : best;
        at = takes ? otherAt : at;
    }
    return __shfl_sync(kFull, at, 0);
}

struct Trail
{
    Index * out;
    unsigned char * visited;
    Index written;
};

__device__ void keep(Trail & trail, Index index, int lane)
{
    const bool fresh = trail.visited == nullptr || trail.visited[index] == 0;
    __syncwarp();
    if (lane == 0 && fresh) {
        if (trail.visited != nullptr) {
            trail.visited[index] = 1;
        }
        trail.out[trail.written] = index;
    }
    __syncwarp();
    trail.written += fresh ? 1u : 0u;
}

__device__ Choice stepFrom(const PointSet & set, double epsilon, Index previous, Index current, bool excludePrevious,
                           int lane)
{
    const Turn turn = turnAt(set.walk[previous], set.walk[current], epsilon);
    return warpChoice(set, turn.current, lane, [&](DeviceComplex candidate, Index j) {
        return nextChoice(turn, candidate, j, epsilon, excludePrevious);
    });
}

__device__ WalkEnd walkFrom(const PointSet & set, double epsilon, Index b1, Index maxSteps, bool excludePrevious,
                            int lane, Trail & trail)
{
    const DeviceComplex first = set.walk[b1];
    const Choice second = warpChoice(set, first, lane, [&](DeviceComplex candidate, Index j) {
        return secondChoice(first, candidate, j, epsilon);
    });
    if (!second.found) {
        return WalkEnd::Dead;
    }
    const Index b2 = second.index;
    keep(trail, b1, lane);
    keep(trail, b2, lane);

    Index counter = 2;
    Index previous = b1;
    Index current = b2;
    Choice next = stepFrom(set, epsilon, previous, current, excludePrevious, lane);
    if (!next.found) {
        return WalkEnd::Dead;
    }
    while (b1 != current || b2 != next.index) {
        if (counter + 1 > maxSteps) {
            return WalkEnd::Exceeded;
        }
        keep(trail, next.index, lane);
        ++counter;
        previous = current;
        current = next.index;
        next = stepFrom(set, epsilon, previous, current, excludePrevious, lane);
        if (!next.found) {
            return WalkEnd::Dead;
        }
    }
    return WalkEnd::Closed;
}

struct PartsView
{
    const DeviceComplex * walk;
    const Index * frequencyOf;
    const Index * offsets;
    const GridFrame * frames;
    const double * epsilon;
    const Bits * cells;
    const Index * cellOrder;
    const Index * members;
    const Index * partStarts;
    const Index * rootOffsets;
    Index points;
    Index parts;
    Index * steps;
    unsigned char * visited;
    Index * length;
    int * kind;
    int * fellBack;
};

__global__ void __launch_bounds__(kWalkBlock) walkParts(PartsView v)
{
    const int lane = static_cast<int>(threadIdx.x % 32);
    const Index warps = gridDim.x * kWalkWarps;
    for (Index r = blockIdx.x * kWalkWarps + threadIdx.x / 32; r < v.parts; r += warps) {
        const Index from = v.partStarts[r];
        const Index to = r + 1 < v.parts ? v.partStarts[r + 1] : v.points;
        const Index size = to - from;
        const Index f = v.frequencyOf[v.members[from]];
        const bool single = v.rootOffsets[f + 1] - v.rootOffsets[f] == 1;
        const PointSet set{v.walk, v.cells, v.cellOrder, v.offsets[f], v.offsets[f + 1], v.frames[f]};
        const double epsilon = v.epsilon[f];
        Trail trail{v.steps + 3 * static_cast<std::size_t>(from), nullptr, 0};

        const Index start = firstExtreme(v.walk, v.members, from, to, false, lane);
        const WalkEnd end = walkFrom(set, epsilon, start, 3 * size, false, lane, trail);
        int kind = end == WalkEnd::Closed ? kWalked : kEmpty;
        int fell = 0;
        if (end == WalkEnd::Exceeded) {
            fell = 1;
            if (single) {
                kind = kPendingRaw;
            } else {
                trail = Trail{trail.out, v.visited, 0};
                const Index highest = firstExtreme(v.walk, v.members, from, to, true, lane);
                const WalkEnd relaxed = walkFrom(set, epsilon, highest, 3 * size, true, lane, trail);
                kind = relaxed == WalkEnd::Closed ? kRelaxed : relaxed == WalkEnd::Dead ? kEmpty : kTruncated;
            }
        }
        if (lane == 0) {
            v.kind[r] = kind;
            v.length[r] = trail.written;
            v.fellBack[r] = fell;
        }
    }
}

struct RawView
{
    const DeviceComplex * walk;
    const Bits * cells;
    const Index * order;
    Index plants;
    const Index * frequency;
    const Index * part;
    const GridFrame * frames;
    const double * epsilon;
    Index pending;
    Index * out;
    unsigned char * visited;
    Index * length;
    int * kind;
};

__global__ void __launch_bounds__(kWalkBlock) walkRaw(RawView v)
{
    const int lane = static_cast<int>(threadIdx.x % 32);
    const Index warps = gridDim.x * kWalkWarps;
    for (Index k = blockIdx.x * kWalkWarps + threadIdx.x / 32; k < v.pending; k += warps) {
        const Index f = v.frequency[k];
        const Index first = k * v.plants;
        const PointSet set{v.walk, v.cells, v.order, first, first + v.plants, v.frames[f]};
        Trail trail{v.out + first, v.visited, 0};
        const Index highest = firstExtreme(v.walk, nullptr, first, first + v.plants, true, lane);
        const WalkEnd end = walkFrom(set, v.epsilon[f], highest, 3 * v.plants, true, lane, trail);
        if (lane == 0) {
            v.kind[v.part[k]] = end == WalkEnd::Closed ? kRelaxed : end == WalkEnd::Dead ? kEmpty : kTruncated;
            v.length[v.part[k]] = trail.written;
        }
    }
}

__global__ void rawProjection(const DeviceComplex * cloud, const Index * frequency, Index plants, Index n,
                              bool nichols, const int * shift, const double * cut, double dbPerDegree,
                              DeviceComplex * walk)
{
    const Index g = blockIdx.x * blockDim.x + threadIdx.x;
    if (g < n) {
        const Index k = g / plants;
        const Index f = frequency[k];
        const DeviceComplex z = cloud[static_cast<std::size_t>(f) * plants + (g - k * plants)];
        walk[g] = nichols ? nicholsPoint(z, dbPerDegree, shift[f] != 0, cut[f]) : z;
    }
}

__global__ void rawCellKeys(const DeviceComplex * walk, const Index * frequency, const GridFrame * frames,
                            Index plants, Index n, Bits * keys, Index * order)
{
    const Index g = blockIdx.x * blockDim.x + threadIdx.x;
    if (g < n) {
        keys[g] = cellKeyOf(frames[frequency[g / plants]], walk[g]);
        order[g] = g;
    }
}

__global__ void rawSlotKeys(const Index * order, Index plants, Index n, Index * keys)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < n) {
        keys[t] = order[t] / plants;
    }
}

__global__ void rawSortedCells(const DeviceComplex * walk, const Index * frequency, const GridFrame * frames,
                               const Index * order, Index plants, Index n, Bits * cells)
{
    const Index t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t < n) {
        const Index g = order[t];
        cells[t] = cellKeyOf(frames[frequency[g / plants]], walk[g]);
    }
}

__device__ bool singlePart(const Index * frequencyOf, const Index * members, const Index * rootOffsets, Index from,
                           Index & f)
{
    f = frequencyOf[members[from]];
    return rootOffsets[f + 1] - rootOffsets[f] == 1;
}

__global__ void outLengths(const Index * partStarts, const Index * members, const Index * frequencyOf,
                           const Index * rootOffsets, const int * kind, const Index * length, Index points,
                           Index parts, Index * out)
{
    const Index r = blockIdx.x * blockDim.x + threadIdx.x;
    if (r < parts) {
        const Index from = partStarts[r];
        const Index to = r + 1 < parts ? partStarts[r + 1] : points;
        Index f = 0;
        const bool single = singlePart(frequencyOf, members, rootOffsets, from, f);
        const int k = kind[r];
        out[r] = k == kWalked || k == kRelaxed ? length[r] : k == kEmpty && !single ? to - from : 0u;
    } else if (r == parts) {
        out[r] = 0u;
    }
}

struct GatherView
{
    const DeviceComplex * points;
    const DeviceComplex * cloud;
    const Index * partStarts;
    const Index * members;
    const Index * frequencyOf;
    const Index * rootOffsets;
    const int * kind;
    const Index * steps;
    const Index * rawSlot;
    const Index * rawOut;
    const Index * places;
    Index plants;
    Index parts;
    DeviceComplex * out;
};

__global__ void __launch_bounds__(kWalkBlock) gatherParts(GatherView v)
{
    const int lane = static_cast<int>(threadIdx.x % 32);
    const Index warps = gridDim.x * kWalkWarps;
    for (Index r = blockIdx.x * kWalkWarps + threadIdx.x / 32; r < v.parts; r += warps) {
        const Index from = v.partStarts[r];
        Index f = 0;
        const bool single = singlePart(v.frequencyOf, v.members, v.rootOffsets, from, f);
        const int kind = v.kind[r];
        const Index base = v.places[r];
        const Index count = v.places[r + 1] - base;
        const bool fromRaw = kind == kRelaxed && single;
        const Index slot = fromRaw ? v.rawSlot[r] : 0u;
        for (Index i = static_cast<Index>(lane); i < count; i += 32) {
            DeviceComplex point;
            if (fromRaw) {
                const Index g = v.rawOut[static_cast<std::size_t>(slot) * v.plants + i];
                point = v.cloud[static_cast<std::size_t>(f) * v.plants + (g - slot * v.plants)];
            } else if (kind == kEmpty) {
                point = v.points[v.members[from + i]];
            } else {
                point = v.points[v.steps[3 * static_cast<std::size_t>(from) + i]];
            }
            v.out[base + i] = point;
        }
    }
}

template <typename Kernel>
unsigned int walkBlocks(Kernel kernel, std::size_t warpsNeeded)
{
    int perMultiprocessor = 0;
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perMultiprocessor, kernel, kWalkBlock, 0),
          "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
    const std::size_t resident = static_cast<std::size_t>(std::max(perMultiprocessor, 1))
            * static_cast<std::size_t>(std::max(probe().multiprocessors, 1));
    const std::size_t needed = (warpsNeeded + kWalkWarps - 1) / kWalkWarps;
    return static_cast<unsigned int>(std::max<std::size_t>(1, std::min(resident, needed)));
}

struct PartWalk
{
    int kind = kEmpty;
    bool fellBack = false;
    ComplexCloud points;
};

WalkedContour joined(const std::vector<PartWalk> & parts)
{
    WalkedContour out;
    if (parts.size() == 1) {
        out.starts.push_back(0);
        out.fellBack = parts.front().fellBack;
        out.truncated = parts.front().kind == kTruncated;
        if (parts.front().kind == kWalked || parts.front().kind == kRelaxed) {
            out.contour = parts.front().points;
        }
        return out;
    }
    bool anyWalked = false;
    for (const PartWalk & part : parts) {
        out.fellBack = out.fellBack || part.fellBack;
        if (part.kind == kTruncated) {
            out.truncated = true;
            out.contour.clear();
            out.starts.clear();
            return out;
        }
        anyWalked = anyWalked || part.kind == kWalked || part.kind == kRelaxed;
        out.starts.push_back(out.contour.size());
        out.contour.insert(out.contour.end(), part.points.begin(), part.points.end());
    }
    if (!anyWalked) {
        out.contour.clear();
        out.starts.clear();
    }
    return out;
}

void walkOnRaw(Work & work, const DeviceComplex * cloud, Index plants, bool nichols, double dbPerDegree,
               const std::vector<Index> & frequency, const std::vector<Index> & part, Index parts,
               DeviceBuffer<Index> & length, DeviceBuffer<int> & kind, DeviceBuffer<Index> & rawSlot,
               DeviceBuffer<Index> & rawOut, const Stream & stream)
{
    const Index pending = static_cast<Index>(frequency.size());
    const Index n = pending * plants;
    const cudaStream_t s = stream.get();
    DeviceBuffer<Index> frequencyOnCard = uploaded(frequency, stream);
    DeviceBuffer<Index> partOnCard = uploaded(part, stream);
    std::vector<Index> slots(parts, kNoPoint);
    for (Index k = 0; k < pending; ++k) {
        slots[part[k]] = k;
    }
    rawSlot = uploaded(slots, stream);

    DeviceBuffer<DeviceComplex> walk(n, stream);
    DeviceBuffer<Bits> keysA(n, stream);
    DeviceBuffer<Bits> keysB(n, stream);
    DeviceBuffer<Index> valuesA(n, stream);
    DeviceBuffer<Index> valuesB(n, stream);
    DeviceBuffer<Bits> cells(n, stream);
    DeviceBuffer<unsigned char> visited(n, stream);
    rawOut = DeviceBuffer<Index>(n, stream);
    Sorter sorter(n, stream);
    visited.fill(0);

    rawProjection<<<blocks(n), kThreads, 0, s>>>(cloud, frequencyOnCard.data(), plants, n, nichols, work.shift.data(),
                                                 work.cut.data(), dbPerDegree, walk.data());
    checkLaunch("the kernel of the projection of the swept clouds");
    Bits * keys = keysA.data();
    Index * order = valuesA.data();
    Index * spare = valuesB.data();
    rawCellKeys<<<blocks(n), kThreads, 0, s>>>(walk.data(), frequencyOnCard.data(), work.frames.data(), plants, n,
                                               keys, order);
    checkLaunch("the kernel of the cells of the swept clouds");
    sorter.sort(keys, keysB.data(), order, spare, n, 2 * kCellBits);
    Index * slotKeys = reinterpret_cast<Index *>(keysA.data());
    rawSlotKeys<<<blocks(n), kThreads, 0, s>>>(order, plants, n, slotKeys);
    checkLaunch("the kernel of the slots of the swept clouds");
    sorter.sort(slotKeys, reinterpret_cast<Index *>(keysB.data()), order, spare, n, bitsFor(pending));
    rawSortedCells<<<blocks(n), kThreads, 0, s>>>(walk.data(), frequencyOnCard.data(), work.frames.data(), order,
                                                  plants, n, cells.data());
    checkLaunch("the kernel of the sorted cells of the swept clouds");

    const RawView view{walk.data(), cells.data(), order, plants, frequencyOnCard.data(), partOnCard.data(),
                       work.frames.data(), work.epsilon.data(), pending, rawOut.data(), visited.data(),
                       length.data(), kind.data()};
    walkRaw<<<walkBlocks(walkRaw, pending), kWalkBlock, 0, s>>>(view);
    checkLaunch("the kernel of the relaxed walks over the swept clouds");
    stream.synchronize();
}

struct HostSet
{
    const ComplexCloud & walk;
    const std::vector<Bits> & cells;
    const std::vector<Index> & order;
    GridFrame frame;
};

DeviceComplex onCard(const std::complex<double> & z)
{
    return DeviceComplex(z.real(), z.imag());
}

template <typename Weigh>
Choice hostChoice(const HostSet & set, DeviceComplex centre, Weigh weigh)
{
    Choice best;
    for (const auto & [lo, hi] : hostColumns(set.frame, set.cells, centre)) {
        for (Index place = lo; place < hi; ++place) {
            const Index j = set.order[place];
            const Choice mine = weigh(onCard(set.walk[j]), j);
            if (better(mine, best)) {
                best = mine;
            }
        }
    }
    return best;
}

WalkEnd hostWalk(const HostSet & set, double epsilon, Index b1, Index maxSteps, bool excludePrevious,
                 std::vector<char> * visited, std::vector<Index> & out)
{
    out.clear();
    const auto keep = [&](Index index) {
        if (visited == nullptr || (*visited)[index] == 0) {
            if (visited != nullptr) {
                (*visited)[index] = 1;
            }
            out.push_back(index);
        }
    };
    const auto stepFrom = [&](Index previous, Index current) {
        const Turn turn = turnAt(onCard(set.walk[previous]), onCard(set.walk[current]), epsilon);
        return hostChoice(set, turn.current, [&](DeviceComplex candidate, Index j) {
            return nextChoice(turn, candidate, j, epsilon, excludePrevious);
        });
    };
    const DeviceComplex first = onCard(set.walk[b1]);
    const Choice second = hostChoice(set, first, [&](DeviceComplex candidate, Index j) {
        return secondChoice(first, candidate, j, epsilon);
    });
    if (!second.found) {
        return WalkEnd::Dead;
    }
    const Index b2 = second.index;
    keep(b1);
    keep(b2);
    Index counter = 2;
    Index previous = b1;
    Index current = b2;
    Choice next = stepFrom(previous, current);
    if (!next.found) {
        return WalkEnd::Dead;
    }
    while (b1 != current || b2 != next.index) {
        if (counter + 1 > maxSteps) {
            return WalkEnd::Exceeded;
        }
        keep(next.index);
        ++counter;
        previous = current;
        current = next.index;
        next = stepFrom(previous, current);
        if (!next.found) {
            return WalkEnd::Dead;
        }
    }
    return WalkEnd::Closed;
}

Index hostExtreme(const ComplexCloud & walk, const std::vector<Index> & list, bool imaginary)
{
    Index at = list.front();
    for (const Index i : list) {
        const double value = imaginary ? walk[i].imag() : walk[i].real();
        const double best = imaginary ? walk[at].imag() : walk[at].real();
        if (value > best) {
            at = i;
        }
    }
    return at;
}

}

WalkedOnCard walkedOnCard(const DeviceClouds & clouds, const std::vector<double> & epsilon, HullMetric metric,
                          double dbPerDegree, const CancellationToken * cancellation)
{
    WalkedOnCard result;
    const std::size_t frequencyCount = clouds.frequencies();
    const std::size_t plants = clouds.plants();
    result.walks.assign(frequencyCount, {});
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
    const std::size_t batch = batchFrequencies(frequencyCount, plants, Work::kBytesPerPoint + kWalkBytesPerPoint);
    const std::size_t points = batch * plants;
    Work work(points, batch, nichols, stream);
    DeviceBuffer<Index> steps(3 * points, stream);
    DeviceBuffer<unsigned char> visited(points, stream);
    DeviceBuffer<Index> length(points, stream);
    DeviceBuffer<int> kind(points, stream);
    DeviceBuffer<int> fellBack(points, stream);
    DeviceBuffer<Index> outLength(points + 1, stream);
    DeviceBuffer<Index> places(points + 1, stream);

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

        visited.fill(0);
        const PartsView view{work.walk.data(), work.frequencyOf.data(), work.offsets.data(), work.frames.data(),
                             work.epsilon.data(), work.cells.data(), work.cellOrder.data(), work.members.data(),
                             work.starts.data(), work.rootOffsets.data(), m, parts, steps.data(), visited.data(),
                             length.data(), kind.data(), fellBack.data()};
        walkParts<<<walkBlocks(walkParts, parts), kWalkBlock, 0, s>>>(view);
        checkLaunch("the kernel of the walks");

        std::vector<int> kinds(parts);
        std::vector<Index> rootOffsets(count + 1);
        check(cudaMemcpyAsync(kinds.data(), kind.data(), parts * sizeof(int), cudaMemcpyDeviceToHost, s),
              "cudaMemcpyAsync of the kinds of the walks");
        check(cudaMemcpyAsync(rootOffsets.data(), work.rootOffsets.data(), (count + 1) * sizeof(Index),
                              cudaMemcpyDeviceToHost, s), "cudaMemcpyAsync of the offsets of the parts");
        if (!stream.finish(cancellation, nullptr)) {
            result.cancelled = true;
            return result;
        }

        std::vector<Index> pendingFrequency;
        std::vector<Index> pendingPart;
        for (Index f = 0; f < count; ++f) {
            for (Index r = rootOffsets[f]; r < rootOffsets[f + 1]; ++r) {
                if (kinds[r] == kPendingRaw) {
                    pendingFrequency.push_back(f);
                    pendingPart.push_back(r);
                }
            }
        }
        DeviceBuffer<Index> rawSlot;
        DeviceBuffer<Index> rawOut;
        if (!pendingPart.empty()) {
            walkOnRaw(work, cloud, static_cast<Index>(plants), nichols, dbPerDegree, pendingFrequency, pendingPart,
                      parts, length, kind, rawSlot, rawOut, stream);
        }

        outLengths<<<blocks(parts + 1), kThreads, 0, s>>>(work.starts.data(), work.members.data(),
                                                          work.frequencyOf.data(), work.rootOffsets.data(),
                                                          kind.data(), length.data(), m, parts, outLength.data());
        checkLaunch("the kernel of the lengths of the contours");
        work.sorter.scan(outLength.data(), places.data(), parts + 1);
        Index total = 0;
        check(cudaMemcpyAsync(&total, places.data() + parts, sizeof total, cudaMemcpyDeviceToHost, s),
              "cudaMemcpyAsync of the length of the contours");
        stream.synchronize();

        DeviceBuffer<DeviceComplex> gathered(total, stream);
        const GatherView gather{work.cloudPoints.data(), cloud, work.starts.data(), work.members.data(),
                                work.frequencyOf.data(), work.rootOffsets.data(), kind.data(), steps.data(),
                                rawSlot.data(), rawOut.data(), places.data(), static_cast<Index>(plants), parts,
                                gathered.data()};
        gatherParts<<<walkBlocks(gatherParts, parts), kWalkBlock, 0, s>>>(gather);
        checkLaunch("the kernel of the gathering of the contours");
        stream.synchronize();
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        std::vector<int> falls(parts);
        std::vector<Index> placesOnHost(parts + 1);
        std::vector<DeviceComplex> contourPoints(total);
        check(cudaMemcpyAsync(kinds.data(), kind.data(), parts * sizeof(int), cudaMemcpyDeviceToHost, s),
              "cudaMemcpyAsync of the kinds of the walks");
        check(cudaMemcpyAsync(falls.data(), fellBack.data(), parts * sizeof(int), cudaMemcpyDeviceToHost, s),
              "cudaMemcpyAsync of the fallbacks of the walks");
        check(cudaMemcpyAsync(placesOnHost.data(), places.data(), (parts + 1) * sizeof(Index),
                              cudaMemcpyDeviceToHost, s), "cudaMemcpyAsync of the places of the contours");
        if (total != 0) {
            check(cudaMemcpyAsync(contourPoints.data(), gathered.data(), total * sizeof(DeviceComplex),
                                  cudaMemcpyDeviceToHost, s), "cudaMemcpyAsync of the contours");
        }
        stream.synchronize();

        for (Index f = 0; f < count; ++f) {
            std::vector<PartWalk> walks;
            for (Index r = rootOffsets[f]; r < rootOffsets[f + 1]; ++r) {
                PartWalk walk;
                walk.kind = kinds[r];
                walk.fellBack = falls[r] != 0;
                walk.points.resize(placesOnHost[r + 1] - placesOnHost[r]);
                std::memcpy(static_cast<void *>(walk.points.data()), contourPoints.data() + placesOnHost[r],
                            walk.points.size() * sizeof(DeviceComplex));
                walks.push_back(std::move(walk));
            }
            result.walks[first + f] = joined(walks);
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }
    return result;
}

std::vector<WalkedContour> walkedOnProcessor(const CloudSet & clouds, const std::vector<double> & epsilon,
                                             HullMetric metric, double dbPerDegree)
{
    std::vector<WalkedContour> walks;
    for (std::size_t f = 0; f < clouds.size(); ++f) {
        const ComplexCloud & cloud = clouds[f];
        if (cloud.empty()) {
            walks.emplace_back();
            continue;
        }
        const HostPrepared host = hostPrepared(cloud, epsilon.at(f), metric, dbPerDegree);
        const SortedCloud & sorted = host.prepared.sorted;
        const CloudParts & parts = host.prepared.parts;
        const HostSet set{sorted.walk, host.cells, host.cellOrder, host.frame};
        const bool single = parts.starts.size() == 1;
        std::vector<char> visited(sorted.points.size(), 0);

        std::vector<PartWalk> partWalks;
        for (std::size_t p = 0; p < parts.starts.size(); ++p) {
            const std::size_t end = p + 1 < parts.starts.size() ? parts.starts[p + 1] : parts.members.size();
            std::vector<Index> members;
            for (std::size_t k = parts.starts[p]; k < end; ++k) {
                members.push_back(static_cast<Index>(parts.members[k]));
            }
            const Index size = static_cast<Index>(members.size());
            PartWalk walk;
            std::vector<Index> list;
            const WalkEnd faithful = hostWalk(set, epsilon.at(f), hostExtreme(sorted.walk, members, false), 3 * size,
                                              false, nullptr, list);
            walk.kind = faithful == WalkEnd::Closed ? kWalked : kEmpty;
            const ComplexCloud * source = &sorted.points;
            if (faithful == WalkEnd::Exceeded) {
                walk.fellBack = true;
                WalkEnd relaxed = WalkEnd::Dead;
                if (single) {
                    ComplexCloud rawWalk;
                    for (const std::complex<double> & z : cloud) {
                        const DeviceComplex w = metric == HullMetric::Nichols
                                ? nicholsPoint(onCard(z), dbPerDegree, host.shift, host.cut) : onCard(z);
                        rawWalk.emplace_back(w.real(), w.imag());
                    }
                    std::vector<Bits> rawCells;
                    std::vector<Index> rawOrder;
                    hostCells(host.frame, rawWalk, rawCells, rawOrder);
                    const HostSet raw{rawWalk, rawCells, rawOrder, host.frame};
                    std::vector<Index> all(cloud.size());
                    for (Index i = 0; i < all.size(); ++i) {
                        all[i] = i;
                    }
                    std::vector<char> rawVisited(cloud.size(), 0);
                    relaxed = hostWalk(raw, epsilon.at(f), hostExtreme(rawWalk, all, true),
                                       3 * static_cast<Index>(cloud.size()), true, &rawVisited, list);
                    source = &cloud;
                } else {
                    relaxed = hostWalk(set, epsilon.at(f), hostExtreme(sorted.walk, members, true), 3 * size, true,
                                       &visited, list);
                }
                walk.kind = relaxed == WalkEnd::Closed ? kRelaxed : relaxed == WalkEnd::Dead ? kEmpty : kTruncated;
            }
            if (walk.kind == kWalked || walk.kind == kRelaxed) {
                for (const Index i : list) {
                    walk.points.push_back((*source)[i]);
                }
            } else if (walk.kind == kEmpty && !single) {
                for (const Index i : members) {
                    walk.points.push_back(sorted.points[i]);
                }
            }
            partWalks.push_back(std::move(walk));
        }
        walks.push_back(joined(partWalks));
    }
    return walks;
}

}
