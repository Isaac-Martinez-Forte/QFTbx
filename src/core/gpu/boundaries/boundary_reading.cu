/**
 * @file
 * @brief The kernels that read the sheets on the card, and their run.
 *
 * After a batch's sheets (SheetBatch), every reading of the batch goes
 * through:
 * - classifyCells() and markStarts(): one thread per cell, column after
 *   column, so a warp's vote is one word of a bitmap; the second also counts
 *   the cells that can start a trace, one atomic addition per warp;
 * - countSpans() and fillSpans(): one thread per column, which reads it up
 *   the magnitudes; the first counts its intervals, the processor adds them
 *   up into where each column writes, and the second writes them;
 * - traceSheets(): one warp per reading, since a trace depends on the ones
 *   before it; the warp looks for the next start 32 words at a time, and at
 *   each step of a walk eight of its threads try the eight neighbours at
 *   once, the first that can go winning, as on the processor;
 * - labelTraces(): one block per trace, which finds the probe under the
 *   trace's highest point and the extremes over the frequency's template.
 * The sizes the processor needs to reserve, the cells that can start a
 * trace, the intervals of each column and the traces of each reading, come
 * back between the kernels; what was read comes back at the end, and the
 * sheets never do.
 */

#include "src/core/gpu/boundaries/boundary_reading.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include <cub/block/block_reduce.cuh>
#include <cuda_runtime.h>

#include "src/core/gpu/boundaries/sheet_batch.cuh"
#include "src/core/gpu/boundaries/sheet_bounds.cuh"
#include "src/core/gpu/boundaries/sheet_reading.cuh"
#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/cuda_error.h"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/launch.cuh"

namespace qftbx::gpu {

namespace {

constexpr std::int32_t kCellsBlock = 256;
constexpr std::int32_t kColumnsBlock = 128;
constexpr std::int32_t kLabelBlock = 128;
constexpr unsigned int kFullWarp = 0xffffffffu;

struct ReadingView
{
    std::int32_t columns;
    std::int32_t rows;
    std::int64_t words;
    const std::int32_t * frequency;
    const std::int32_t * sheet;
    const double * height;
    const std::int64_t * sheetAt;
    const double * sheets;
    std::uint32_t * high;
    std::uint32_t * border;
    std::uint32_t * visited;
    std::int32_t * borderCount;
    std::int32_t * spanCount;
    const std::int64_t * spanAt;
    double * lo;
    double * hi;
    const std::int64_t * pointAt;
    const std::int32_t * capacity;
    std::int32_t * x;
    std::int32_t * y;
    std::int32_t * start;
    std::int32_t * pointCount;
    std::int32_t * traceCount;
    std::int32_t * overflow;
    ColumnScale columnScale;
    int * cancel;
};

struct TraceView
{
    const std::int32_t * reading;
    const std::int64_t * begin;
    const std::int64_t * end;
    char * label;
};

__global__ void __launch_bounds__(kCellsBlock) classifyCells(ReadingView v)
{
    const std::int32_t r = static_cast<std::int32_t>(blockIdx.y);
    const std::int64_t cells = static_cast<std::int64_t>(v.columns) * v.rows;
    const std::int64_t index = static_cast<std::int64_t>(blockIdx.x) * kCellsBlock + threadIdx.x;
    bool high = false;
    if (index < cells) {
        const std::int32_t x = static_cast<std::int32_t>(index / v.rows);
        const std::int32_t y = static_cast<std::int32_t>(index % v.rows);
        high = v.sheets[v.sheetAt[r] + static_cast<std::int64_t>(y) * v.columns + x] >= v.height[r];
    }
    const unsigned int word = __ballot_sync(kFullWarp, high);
    if (threadIdx.x % 32 == 0 && index < cells) {
        v.high[r * v.words + (index >> 5)] = word;
    }
}

__global__ void __launch_bounds__(kCellsBlock) markStarts(ReadingView v)
{
    const std::int32_t r = static_cast<std::int32_t>(blockIdx.y);
    const std::int64_t cells = static_cast<std::int64_t>(v.columns) * v.rows;
    const std::int64_t index = static_cast<std::int64_t>(blockIdx.x) * kCellsBlock + threadIdx.x;
    bool starts = false;
    if (index < cells) {
        starts = startsTrace(v.high + r * v.words, static_cast<std::int32_t>(index / v.rows),
                             static_cast<std::int32_t>(index % v.rows), v.columns, v.rows);
    }
    const unsigned int word = __ballot_sync(kFullWarp, starts);
    if (threadIdx.x % 32 == 0 && index < cells) {
        v.border[r * v.words + (index >> 5)] = word;
        if (word != 0) {
            atomicAdd(v.borderCount + r, __popc(word));
        }
    }
}

__global__ void __launch_bounds__(kColumnsBlock) countSpans(ReadingView v)
{
    const std::int32_t r = static_cast<std::int32_t>(blockIdx.y);
    const std::int32_t column = static_cast<std::int32_t>(blockIdx.x) * kColumnsBlock + static_cast<std::int32_t>(threadIdx.x);
    if (column >= v.columns) {
        return;
    }
    v.spanCount[static_cast<std::int64_t>(r) * v.columns + column] =
            readColumn<false>(v.sheets + v.sheetAt[r], v.columns, v.rows, column, v.height[r], v.columnScale, nullptr,
                              nullptr);
}

__global__ void __launch_bounds__(kColumnsBlock) fillSpans(ReadingView v)
{
    const std::int32_t r = static_cast<std::int32_t>(blockIdx.y);
    const std::int32_t column = static_cast<std::int32_t>(blockIdx.x) * kColumnsBlock + static_cast<std::int32_t>(threadIdx.x);
    if (column >= v.columns) {
        return;
    }
    const std::int64_t at = v.spanAt[static_cast<std::int64_t>(r) * v.columns + column];
    readColumn<true>(v.sheets + v.sheetAt[r], v.columns, v.rows, column, v.height[r], v.columnScale, v.lo + at,
                     v.hi + at);
}

__device__ void walkOnCard(SheetBits & bits, std::int32_t x, std::int32_t y, TraceOutput & out, std::int32_t lane)
{
    const std::int32_t first = out.points;
    while (true) {
        std::int32_t nextX = 0;
        std::int32_t nextY = 0;
        const bool can = lane < 8 && stepTo(bits, x, y, 15 - lane, nextX, nextY);
        const unsigned int ways = __ballot_sync(kFullWarp, can);
        emitNode(out, x, y, lane == 0);
        if (ways == 0) {
            break;
        }
        const std::int32_t way = __ffs(static_cast<int>(ways)) - 1;
        nextX = __shfl_sync(kFullWarp, nextX, way);
        nextY = __shfl_sync(kFullWarp, nextY, way);
        if (lane == 0) {
            markVisited(bits.visited, cellIndex(x, y, bits.rows));
        }
        __syncwarp();
        x = nextX;
        y = nextY;
    }
    closeTrace(out, first, lane == 0);
    __syncwarp();
}

__global__ void __launch_bounds__(32) traceSheets(ReadingView v)
{
    const std::int32_t r = static_cast<std::int32_t>(blockIdx.x);
    const std::int32_t lane = static_cast<std::int32_t>(threadIdx.x);
    if (cancelRequested(v.cancel)) {
        return;
    }
    SheetBits bits{v.high + r * v.words, v.border + r * v.words, v.visited + r * v.words, v.columns, v.rows};
    const std::int64_t at = v.pointAt[r];
    TraceOutput out{v.x + at, v.y + at, v.start + at, v.capacity[r], 0, 0, false};

    std::int64_t cursor = 0;
    while (true) {
        std::int64_t base = cursor >> 5;
        unsigned int found = 0;
        std::uint32_t pending = 0;
        while (base < v.words) {
            const std::int64_t w = base + lane;
            pending = w < v.words ? candidates(bits, w, cursor) : 0u;
            found = __ballot_sync(kFullWarp, pending != 0);
            if (found != 0) {
                break;
            }
            base += 32;
        }
        if (found == 0) {
            break;
        }
        const std::int32_t source = __ffs(static_cast<int>(found)) - 1;
        const std::uint32_t word = __shfl_sync(kFullWarp, pending, source);
        const std::int64_t cell = (base + source) * 32 + (__ffs(static_cast<int>(word)) - 1);
        walkOnCard(bits, static_cast<std::int32_t>(cell / v.rows), static_cast<std::int32_t>(cell % v.rows), out, lane);
        cursor = cell + 1;
    }
    if (lane == 0) {
        v.pointCount[r] = out.points;
        v.traceCount[r] = out.traces;
        v.overflow[r] = out.overflow ? 1 : 0;
    }
}

struct Highest
{
    std::int32_t y;
    std::int64_t index;
};

struct HigherFirst
{
    __device__ Highest operator()(const Highest & a, const Highest & b) const
    {
        return (a.y > b.y || (a.y == b.y && a.index < b.index)) ? a : b;
    }
};

struct Seen
{
    double nearest;
    double farthest;
    double ratio;
};

struct Widest
{
    __device__ Seen operator()(const Seen & a, const Seen & b) const
    {
        return {a.nearest <= b.nearest ? a.nearest : b.nearest, a.farthest >= b.farthest ? a.farthest : b.farthest,
                a.ratio >= b.ratio ? a.ratio : b.ratio};
    }
};

__device__ inline Seen seenFrom(double d, double radius)
{
    const double ratio = radius / d;
    return {d, d, ratio == ratio ? ratio : 0.0};
}

__global__ void __launch_bounds__(kLabelBlock) labelTraces(ReadingView v, TraceView t, BatchView b, NodeScale scale)
{
    using FindHighest = cub::BlockReduce<Highest, kLabelBlock>;
    using FindSeen = cub::BlockReduce<Seen, kLabelBlock>;
    __shared__ union {
        typename FindHighest::TempStorage highest;
        typename FindSeen::TempStorage seen;
    } storage;
    __shared__ Highest probe;

    if (cancelRequested(v.cancel)) {
        return;
    }
    const std::int64_t trace = blockIdx.x;
    const std::int32_t r = t.reading[trace];
    const std::int64_t begin = t.begin[trace];
    const std::int64_t end = t.end[trace];

    Highest mine{-1, end};
    for (std::int64_t k = begin + threadIdx.x; k < end; k += kLabelBlock) {
        mine = HigherFirst{}(mine, Highest{v.y[k], k});
    }
    const Highest highest = FindHighest(storage.highest).Reduce(mine, HigherFirst{});
    if (threadIdx.x == 0) {
        probe = highest;
    }
    __syncthreads();

    const DeviceComplex L = probeLoop(scale, v.x[probe.index], probe.y, probe.index == begin);
    const std::int32_t u = v.frequency[r];
    const std::size_t first = b.offset[u];
    const std::int32_t count = b.count[u];
    Seen seen{kInfinity, 0.0, 0.0};
    for (std::int32_t i = static_cast<std::int32_t>(threadIdx.x); i < count; i += kLabelBlock) {
        const DeviceComplex q = b.q[first + static_cast<std::size_t>(i)];
        const double d = ::hypot(q.real() + L.real(), q.imag() + L.imag());
        seen = Widest{}(seen, seenFrom(d, b.radius[first + static_cast<std::size_t>(i)]));
    }
    __syncthreads();
    const Seen all = FindSeen(storage.seen).Reduce(seen, Widest{});
    if (threadIdx.x == 0) {
        t.label[trace] = allowedBelow(static_cast<std::size_t>(v.sheet[r]), v.height[r], L, b.nominalMagnitude[u],
                                      all.nearest, all.farthest, all.ratio)
                ? 1 : 0;
    }
}

double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

std::int64_t wordsOf(std::size_t columns, std::size_t rows)
{
    return static_cast<std::int64_t>((columns * rows + 31) / 32);
}

std::int32_t capacityOf(std::int32_t starts, std::size_t columns, std::size_t rows)
{
    return 2 * (starts + 2 * static_cast<std::int32_t>(columns + rows)) + 1;
}

NodeScale nodeScaleOf(Range phaseRange, std::size_t columns, Range magnitudeRange, std::size_t rows)
{
    return {phaseRange.width(), phaseRange.min, static_cast<double>(static_cast<std::int32_t>(columns) - 1),
            magnitudeRange.width(), magnitudeRange.min, static_cast<double>(static_cast<std::int32_t>(rows) - 1)};
}

ColumnScale columnScaleOf(Range magnitudeRange, std::size_t rows)
{
    const std::int32_t count = static_cast<std::int32_t>(rows);
    return {magnitudeRange.min, count > 1 ? magnitudeRange.width() / (count - 1) : 0.0};
}

template <typename T>
std::vector<T> downloaded(const DeviceBuffer<T> & buffer, const Stream & stream)
{
    std::vector<T> host(buffer.size());
    buffer.download(host.data());
    stream.synchronize();
    return host;
}

std::vector<double> rowMajor(const BoundarySheet & sheet)
{
    std::vector<double> values;
    for (const std::vector<double> & row : sheet) {
        values.insert(values.end(), row.begin(), row.end());
    }
    return values;
}

}

BoundariesOnCard boundariesOnCard(const SheetProblem & problem,
                                  const std::vector<std::vector<ReadingRequest>> & requests,
                                  Range phaseRange, Range magnitudeRange, const CancellationToken * cancellation)
{
    BoundariesOnCard result;
    const std::size_t frequencyCount = problem.templates->size();
    result.readings.assign(frequencyCount, {});
    if (frequencyCount == 0 || problem.rowMagnitudes.empty() || problem.cosines.empty()) {
        return result;
    }

    useProbedDevice();
    SheetRun run(problem);
    const std::size_t rows = run.rows;
    const std::size_t columns = run.columns;
    const std::int64_t words = wordsOf(columns, rows);
    const NodeScale nodeScale = nodeScaleOf(phaseRange, columns, magnitudeRange, rows);
    const Stream & stream = run.stream;

    const auto perFrequency = [&](std::size_t u) {
        return requests.at(u).size()
                * (3 * static_cast<std::size_t>(words) * sizeof(std::uint32_t) + 2 * columns * sizeof(std::int64_t)
                   + 2 * (rows + columns) * (2 * sizeof(double) + 3 * sizeof(std::int32_t)));
    };

    for (const BatchRange & range : batchesOf(run, perFrequency)) {
        const auto computing = std::chrono::steady_clock::now();
        const SheetBatch batch(run, range);

        std::vector<std::int32_t> frequency;
        std::vector<std::int32_t> sheet;
        std::vector<double> height;
        std::vector<std::int64_t> sheetAt;
        std::vector<std::pair<std::size_t, std::size_t>> owner;
        for (std::size_t u = 0; u < range.count; ++u) {
            const std::vector<ReadingRequest> & asked = requests.at(range.first + u);
            result.readings[range.first + u].assign(asked.size(), {});
            for (std::size_t k = 0; k < asked.size(); ++k) {
                const std::int64_t at = batch.sheetOffset.at(5 * u + asked[k].sheet);
                if (at < 0) {
                    throw CudaError("a reading asks for a sheet that was not computed", 0);
                }
                frequency.push_back(static_cast<std::int32_t>(u));
                sheet.push_back(static_cast<std::int32_t>(asked[k].sheet));
                height.push_back(asked[k].heightDb);
                sheetAt.push_back(at);
                owner.emplace_back(range.first + u, k);
            }
        }
        const std::size_t readings = frequency.size();
        if (readings == 0) {
            if (!run.stream.finish(cancellation, &run.flag)) {
                result.cancelled = true;
                return result;
            }
            ++result.batches;
            continue;
        }

        const DeviceBuffer<std::int32_t> frequencyOnCard = uploaded(frequency, stream);
        const DeviceBuffer<std::int32_t> sheetOnCard = uploaded(sheet, stream);
        const DeviceBuffer<double> heightOnCard = uploaded(height, stream);
        const DeviceBuffer<std::int64_t> sheetAtOnCard = uploaded(sheetAt, stream);
        DeviceBuffer<std::uint32_t> high(readings * static_cast<std::size_t>(words), stream);
        DeviceBuffer<std::uint32_t> border(readings * static_cast<std::size_t>(words), stream);
        DeviceBuffer<std::uint32_t> visited(readings * static_cast<std::size_t>(words), stream);
        DeviceBuffer<std::int32_t> borderCount(readings, stream);
        DeviceBuffer<std::int32_t> spanCount(readings * columns, stream);
        visited.fill(0);
        borderCount.fill(0);

        ReadingView view{};
        view.columns = static_cast<std::int32_t>(columns);
        view.rows = static_cast<std::int32_t>(rows);
        view.words = words;
        view.frequency = frequencyOnCard.data();
        view.sheet = sheetOnCard.data();
        view.height = heightOnCard.data();
        view.sheetAt = sheetAtOnCard.data();
        view.sheets = batch.view.sheets;
        view.high = high.data();
        view.border = border.data();
        view.visited = visited.data();
        view.borderCount = borderCount.data();
        view.spanCount = spanCount.data();
        view.columnScale = columnScaleOf(magnitudeRange, rows);
        view.cancel = run.flag.device();

        const dim3 cellBlocks(blocksFor(columns * rows, kCellsBlock), static_cast<unsigned int>(readings));
        const dim3 columnBlocks(blocksFor(columns, kColumnsBlock), static_cast<unsigned int>(readings));
        classifyCells<<<cellBlocks, kCellsBlock, 0, stream.get()>>>(view);
        checkLaunch("the classification of the cells");
        markStarts<<<cellBlocks, kCellsBlock, 0, stream.get()>>>(view);
        checkLaunch("the starts of the traces");
        countSpans<<<columnBlocks, kColumnsBlock, 0, stream.get()>>>(view);
        checkLaunch("the count of the columns' intervals");
        if (!run.stream.finish(cancellation, &run.flag)) {
            result.cancelled = true;
            return result;
        }

        const std::vector<std::int32_t> starts = downloaded(borderCount, stream);
        const std::vector<std::int32_t> spans = downloaded(spanCount, stream);
        std::vector<std::int64_t> pointAt(readings);
        std::vector<std::int32_t> capacity(readings);
        std::int64_t points = 0;
        for (std::size_t r = 0; r < readings; ++r) {
            pointAt[r] = points;
            capacity[r] = capacityOf(starts[r], columns, rows);
            points += capacity[r];
        }
        std::vector<std::int64_t> spanAt(readings * columns);
        std::int64_t spanTotal = 0;
        for (std::size_t c = 0; c < spanAt.size(); ++c) {
            spanAt[c] = spanTotal;
            spanTotal += spans[c];
        }

        const DeviceBuffer<std::int64_t> pointAtOnCard = uploaded(pointAt, stream);
        const DeviceBuffer<std::int32_t> capacityOnCard = uploaded(capacity, stream);
        const DeviceBuffer<std::int64_t> spanAtOnCard = uploaded(spanAt, stream);
        DeviceBuffer<std::int32_t> x(static_cast<std::size_t>(points), stream);
        DeviceBuffer<std::int32_t> y(static_cast<std::size_t>(points), stream);
        DeviceBuffer<std::int32_t> start(static_cast<std::size_t>(points), stream);
        DeviceBuffer<std::int32_t> pointCount(readings, stream);
        DeviceBuffer<std::int32_t> traceCount(readings, stream);
        DeviceBuffer<std::int32_t> overflow(readings, stream);
        DeviceBuffer<double> lo(static_cast<std::size_t>(spanTotal), stream);
        DeviceBuffer<double> hi(static_cast<std::size_t>(spanTotal), stream);
        view.pointAt = pointAtOnCard.data();
        view.capacity = capacityOnCard.data();
        view.x = x.data();
        view.y = y.data();
        view.start = start.data();
        view.pointCount = pointCount.data();
        view.traceCount = traceCount.data();
        view.overflow = overflow.data();
        view.spanAt = spanAtOnCard.data();
        view.lo = lo.data();
        view.hi = hi.data();

        traceSheets<<<static_cast<unsigned int>(readings), 32, 0, stream.get()>>>(view);
        checkLaunch("the traces of the sheets");
        fillSpans<<<columnBlocks, kColumnsBlock, 0, stream.get()>>>(view);
        checkLaunch("the intervals of the columns");
        if (!run.stream.finish(cancellation, &run.flag)) {
            result.cancelled = true;
            return result;
        }

        const std::vector<std::int32_t> pointsOf = downloaded(pointCount, stream);
        const std::vector<std::int32_t> tracesOf = downloaded(traceCount, stream);
        const std::vector<std::int32_t> overflowed = downloaded(overflow, stream);
        const std::vector<std::int32_t> startsOf = downloaded(start, stream);
        if (std::find(overflowed.begin(), overflowed.end(), 1) != overflowed.end()) {
            throw CudaError("a sheet has more trace points than were reserved for it", 0);
        }
        std::vector<std::int32_t> traceReading;
        std::vector<std::int64_t> traceBegin;
        std::vector<std::int64_t> traceEnd;
        for (std::size_t r = 0; r < readings; ++r) {
            for (std::int32_t k = 0; k < tracesOf[r]; ++k) {
                const std::int64_t from = pointAt[r] + startsOf[static_cast<std::size_t>(pointAt[r] + k)];
                const std::int64_t to = k + 1 < tracesOf[r]
                        ? pointAt[r] + startsOf[static_cast<std::size_t>(pointAt[r] + k + 1)] : pointAt[r] + pointsOf[r];
                traceReading.push_back(static_cast<std::int32_t>(r));
                traceBegin.push_back(from);
                traceEnd.push_back(to);
            }
        }
        DeviceBuffer<char> label(traceReading.size(), stream);
        if (!traceReading.empty()) {
            const DeviceBuffer<std::int32_t> traceReadingOnCard = uploaded(traceReading, stream);
            const DeviceBuffer<std::int64_t> traceBeginOnCard = uploaded(traceBegin, stream);
            const DeviceBuffer<std::int64_t> traceEndOnCard = uploaded(traceEnd, stream);
            const TraceView traceView{traceReadingOnCard.data(), traceBeginOnCard.data(), traceEndOnCard.data(),
                                      label.data()};
            labelTraces<<<static_cast<unsigned int>(traceReading.size()), kLabelBlock, 0, stream.get()>>>(
                    view, traceView, batch.view, nodeScale);
            checkLaunch("the labels of the traces");
            if (!run.stream.finish(cancellation, &run.flag)) {
                result.cancelled = true;
                return result;
            }
        }
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        const std::vector<std::int32_t> xs = downloaded(x, stream);
        const std::vector<std::int32_t> ys = downloaded(y, stream);
        const std::vector<char> labels = downloaded(label, stream);
        const std::vector<double> los = downloaded(lo, stream);
        const std::vector<double> his = downloaded(hi, stream);
        std::size_t traceIndex = 0;
        for (std::size_t r = 0; r < readings; ++r) {
            ReadingOnCard & read = result.readings[owner[r].first][owner[r].second];
            const std::size_t from = static_cast<std::size_t>(pointAt[r]);
            const std::size_t to = from + static_cast<std::size_t>(pointsOf[r]);
            read.x.assign(xs.begin() + static_cast<std::ptrdiff_t>(from), xs.begin() + static_cast<std::ptrdiff_t>(to));
            read.y.assign(ys.begin() + static_cast<std::ptrdiff_t>(from), ys.begin() + static_cast<std::ptrdiff_t>(to));
            for (std::int32_t k = 0; k < tracesOf[r]; ++k) {
                read.traceStart.push_back(startsOf[from + static_cast<std::size_t>(k)]);
                read.allowedBelow.push_back(labels[traceIndex++]);
            }
            read.traceStart.push_back(pointsOf[r]);
            const std::size_t firstColumn = r * columns;
            read.columnBegin.push_back(0);
            for (std::size_t c = 0; c < columns; ++c) {
                read.columnBegin.push_back(read.columnBegin.back() + spans[firstColumn + c]);
            }
            const std::size_t spanFrom = static_cast<std::size_t>(spanAt[firstColumn]);
            const std::size_t spanTo = spanFrom + static_cast<std::size_t>(read.columnBegin.back());
            read.lo.assign(los.begin() + static_cast<std::ptrdiff_t>(spanFrom), los.begin() + static_cast<std::ptrdiff_t>(spanTo));
            read.hi.assign(his.begin() + static_cast<std::ptrdiff_t>(spanFrom), his.begin() + static_cast<std::ptrdiff_t>(spanTo));
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }
    return result;
}

std::vector<std::vector<ReadingOnCard>> readingsOnProcessor(const SheetProblem & problem,
                                                            const std::vector<BoundarySheets> & sheets,
                                                            const std::vector<std::vector<ReadingRequest>> & requests,
                                                            Range phaseRange, Range magnitudeRange)
{
    const std::size_t rows = problem.rowMagnitudes.size();
    const std::size_t columns = problem.cosines.size();
    const std::int64_t words = wordsOf(columns, rows);
    const NodeScale nodeScale = nodeScaleOf(phaseRange, columns, magnitudeRange, rows);
    const ColumnScale columnScale = columnScaleOf(magnitudeRange, rows);
    const std::int32_t width = static_cast<std::int32_t>(columns);
    const std::int32_t height = static_cast<std::int32_t>(rows);

    std::vector<std::vector<ReadingOnCard>> readings(requests.size());
    for (std::size_t u = 0; u < requests.size(); ++u) {
        const std::vector<std::complex<double>> & plants = problem.templates->at(u);
        const DeviceComplex p0(problem.nominal.at(u).real(), problem.nominal.at(u).imag());
        std::vector<DeviceComplex> q(plants.size());
        std::vector<double> radius(plants.size());
        for (std::size_t i = 0; i < plants.size(); ++i) {
            SampleFloats single;
            prepareSample(p0, DeviceComplex(plants[i].real(), plants[i].imag()), q[i], radius[i], single);
        }

        for (const ReadingRequest & asked : requests[u]) {
            const std::vector<double> sheet = rowMajor(sheets.at(u).at(asked.sheet));
            ReadingOnCard read;

            read.columnBegin.push_back(0);
            for (std::int32_t c = 0; c < width; ++c) {
                const std::int32_t count = readColumn<false>(sheet.data(), width, height, c, asked.heightDb, columnScale,
                                                             nullptr, nullptr);
                read.lo.resize(read.lo.size() + static_cast<std::size_t>(count));
                read.hi.resize(read.hi.size() + static_cast<std::size_t>(count));
                readColumn<true>(sheet.data(), width, height, c, asked.heightDb, columnScale,
                                 read.lo.data() + read.columnBegin.back(), read.hi.data() + read.columnBegin.back());
                read.columnBegin.push_back(read.columnBegin.back() + count);
            }

            std::vector<std::uint32_t> high(static_cast<std::size_t>(words), 0u);
            std::vector<std::uint32_t> border(static_cast<std::size_t>(words), 0u);
            std::vector<std::uint32_t> visited(static_cast<std::size_t>(words), 0u);
            for (std::int32_t cx = 0; cx < width; ++cx) {
                for (std::int32_t cy = 0; cy < height; ++cy) {
                    if (sheet[static_cast<std::size_t>(cy) * columns + static_cast<std::size_t>(cx)] >= asked.heightDb) {
                        markVisited(high.data(), cellIndex(cx, cy, height));
                    }
                }
            }
            std::int32_t starts = 0;
            for (std::int32_t cx = 0; cx < width; ++cx) {
                for (std::int32_t cy = 0; cy < height; ++cy) {
                    if (startsTrace(high.data(), cx, cy, width, height)) {
                        markVisited(border.data(), cellIndex(cx, cy, height));
                        ++starts;
                    }
                }
            }
            const std::int32_t capacity = capacityOf(starts, columns, rows);
            read.x.resize(static_cast<std::size_t>(capacity));
            read.y.resize(static_cast<std::size_t>(capacity));
            std::vector<std::int32_t> start(static_cast<std::size_t>(capacity));
            SheetBits bits{high.data(), border.data(), visited.data(), width, height};
            TraceOutput out{read.x.data(), read.y.data(), start.data(), capacity, 0, 0, false};
            std::int64_t cursor = 0;
            for (std::int64_t w = 0; w < words; ++w) {
                std::uint32_t pending = candidates(bits, w, cursor);
                while (pending != 0) {
                    std::int32_t bit = 0;
                    while (((pending >> bit) & 1u) == 0) {
                        ++bit;
                    }
                    const std::int64_t cell = w * 32 + bit;
                    walkOnProcessor(bits, static_cast<std::int32_t>(cell / height), static_cast<std::int32_t>(cell % height),
                                    out);
                    cursor = cell + 1;
                    pending = candidates(bits, w, cursor);
                }
            }
            read.x.resize(static_cast<std::size_t>(out.points));
            read.y.resize(static_cast<std::size_t>(out.points));
            read.traceStart.assign(start.begin(), start.begin() + out.traces);
            read.traceStart.push_back(out.points);

            for (std::int32_t k = 0; k < out.traces; ++k) {
                const std::int32_t from = read.traceStart[static_cast<std::size_t>(k)];
                const std::int32_t to = read.traceStart[static_cast<std::size_t>(k) + 1];
                std::int32_t best = from;
                for (std::int32_t p = from; p < to; ++p) {
                    best = read.y[static_cast<std::size_t>(p)] > read.y[static_cast<std::size_t>(best)] ? p : best;
                }
                const DeviceComplex L = probeLoop(nodeScale, read.x[static_cast<std::size_t>(best)],
                                                  read.y[static_cast<std::size_t>(best)], best == from);
                double nearest = kInfinity;
                double farthest = 0.0;
                double ratio = 0.0;
                for (std::size_t i = 0; i < q.size(); ++i) {
                    const double d = std::hypot(q[i].real() + L.real(), q[i].imag() + L.imag());
                    const double value = radius[i] / d;
                    nearest = std::min(nearest, d);
                    farthest = std::max(farthest, d);
                    ratio = value == value ? std::max(ratio, value) : ratio;
                }
                read.allowedBelow.push_back(allowedBelow(asked.sheet, asked.heightDb, L, std::abs(problem.nominal.at(u)),
                                                         nearest, farthest, ratio) ? 1 : 0);
            }
            readings[u].push_back(std::move(read));
        }
    }
    return readings;
}

}
