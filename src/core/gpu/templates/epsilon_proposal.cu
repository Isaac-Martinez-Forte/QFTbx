/**
 * @file
 * @brief The kernels of the epsilon proposal and its rounds.
 *
 * A block takes a frequency and grows its tree by Prim's rule, every point
 * outside the tree kept by one thread with its nearest distance to the
 * tree, squared, and a reduction over the block choosing the nearest, the
 * one of smaller index on a tie, at every step; then the block weighs every
 * pair for the diameter. The processor takes the length of the longest edge
 * and of the farthest pair from their points, as the engine measures them.
 * The first epsilon of the climb and its rungs are walked together, since
 * they almost always decide it; the clouds they leave open go on climbing,
 * twice as many epsilons each round, and their rungs are walked once one
 * closes. Every epsilon is walked on its own copy of the cloud.
 */

#include "src/core/gpu/templates/epsilon_proposal.h"

#include <algorithm>
#include <chrono>
#include <complex>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <cuda_runtime.h>
#include <math_constants.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/stream.cuh"
#include "src/core/gpu/templates/contour_walk.h"
#include "src/core/gpu/templates/device_clouds.cuh"
#include "src/core/gpu/templates/walk_work.cuh"
#include "src/core/templates/epsilon_climb.h"
#include "src/core/templates/template_engine.h"

namespace qftbx::gpu {

namespace {

constexpr int kSpanWarps = 8;
constexpr int kSpanThreads = 32 * kSpanWarps;
constexpr std::size_t kSpanBytesPerPoint = sizeof(double) + sizeof(Index);
constexpr std::size_t kWidestClimb = 64;

struct Edge
{
    double length;
    Index to;
    Index from;
};

struct SpanPairs
{
    DeviceComplex longestFrom;
    DeviceComplex longestTo;
    DeviceComplex farthestFrom;
    DeviceComplex farthestTo;
};

QFTBX_HD inline Edge noEdge()
{
    return Edge{0.0, kNoPoint, kNoPoint};
}

QFTBX_HD inline double squaredDistance(DeviceComplex a, DeviceComplex b)
{
    const double dx = a.real() - b.real();
    const double dy = a.imag() - b.imag();
    return dx * dx + dy * dy;
}

QFTBX_HD inline bool shorter(const Edge & a, const Edge & b)
{
    return a.to != kNoPoint && (b.to == kNoPoint || a.length < b.length || (a.length == b.length && a.to < b.to));
}

QFTBX_HD inline bool farther(const Edge & a, const Edge & b)
{
    return a.to != kNoPoint
            && (b.to == kNoPoint || a.length > b.length
                || (a.length == b.length && (a.from < b.from || (a.from == b.from && a.to < b.to))));
}

QFTBX_HD inline Edge nearestOf(double reach, double distance, Index j, bool & closer)
{
    const bool open = reach >= 0.0;
    closer = open && distance < reach;
    return Edge{closer ? distance : reach, open ? j : kNoPoint, kNoPoint};
}

__device__ Edge shuffledDown(const Edge & edge, int d)
{
    Edge other = noEdge();
    other.length = __shfl_down_sync(kFull, edge.length, d);
    other.to = __shfl_down_sync(kFull, edge.to, d);
    other.from = __shfl_down_sync(kFull, edge.from, d);
    return other;
}

template <typename Better>
__device__ Edge blockBest(Edge mine, Edge * shared, Better better)
{
    for (int d = 16; d > 0; d >>= 1) {
        const Edge other = shuffledDown(mine, d);
        if (better(other, mine)) {
            mine = other;
        }
    }
    if (threadIdx.x % 32 == 0) {
        shared[threadIdx.x / 32] = mine;
    }
    __syncthreads();
    Edge best = shared[0];
    for (int w = 1; w < kSpanWarps; ++w) {
        if (better(shared[w], best)) {
            best = shared[w];
        }
    }
    return best;
}

__global__ void __launch_bounds__(kSpanThreads) spanningPairs(const DeviceComplex * walk, const Index * offsets,
                                                               Index frequencies, double * reach, Index * nearest,
                                                               SpanPairs * out)
{
    __shared__ Edge reduced[2][kSpanWarps];
    const auto byShorter = [](const Edge & a, const Edge & b) { return shorter(a, b); };
    const auto byFarther = [](const Edge & a, const Edge & b) { return farther(a, b); };
    const Index t = threadIdx.x;
    int parity = 0;
    for (Index f = blockIdx.x; f < frequencies; f += gridDim.x) {
        const Index first = offsets[f];
        const Index n = offsets[f + 1] - first;
        const DeviceComplex * points = walk + first;
        double * best = reach + first;
        Index * near = nearest + first;
        SpanPairs pairs{};
        if (n >= 2) {
            for (Index j = t; j < n; j += kSpanThreads) {
                best[j] = j == 0 ? -1.0 : CUDART_INF;
                near[j] = 0;
            }
            Edge longest = noEdge();
            Index current = 0;
            for (Index added = 1; added < n; ++added) {
                const DeviceComplex p = points[current];
                Edge mine = noEdge();
                for (Index j = t; j < n; j += kSpanThreads) {
                    bool closer = false;
                    const double distance = squaredDistance(points[j], p);
                    const Edge candidate = nearestOf(best[j], distance, j, closer);
                    if (closer) {
                        best[j] = distance;
                        near[j] = current;
                    }
                    if (shorter(candidate, mine)) {
                        mine = candidate;
                    }
                }
                Edge chosen = blockBest(mine, reduced[parity], byShorter);
                parity ^= 1;
                chosen.from = near[chosen.to];
                if (longest.to == kNoPoint || chosen.length > longest.length) {
                    longest = chosen;
                }
                if (chosen.to % kSpanThreads == t) {
                    best[chosen.to] = -1.0;
                }
                current = chosen.to;
            }

            Edge farthest = noEdge();
            for (Index i = 0; i + 1 < n; ++i) {
                const DeviceComplex p = points[i];
                for (Index j = i + 1 + t; j < n; j += kSpanThreads) {
                    const Edge candidate{squaredDistance(points[j], p), j, i};
                    if (farther(candidate, farthest)) {
                        farthest = candidate;
                    }
                }
            }
            const Edge widest = blockBest(farthest, reduced[parity], byFarther);
            parity ^= 1;
            pairs = SpanPairs{points[longest.from], points[longest.to], points[widest.from], points[widest.to]};
        }
        if (t == 0) {
            out[f] = pairs;
        }
    }
}

__global__ void copiedClouds(const DeviceComplex * clouds, const Index * sources, std::size_t plants, std::size_t n,
                             DeviceComplex * out)
{
    const std::size_t g = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (g < n) {
        const std::size_t k = g / plants;
        out[g] = clouds[static_cast<std::size_t>(sources[k]) * plants + (g - k * plants)];
    }
}

SpanPairs hostPairs(const ComplexCloud & walk)
{
    const Index n = static_cast<Index>(walk.size());
    SpanPairs pairs{};
    if (n < 2) {
        return pairs;
    }
    std::vector<DeviceComplex> points;
    points.reserve(n);
    for (const std::complex<double> & z : walk) {
        points.emplace_back(z.real(), z.imag());
    }
    std::vector<double> best(n, std::numeric_limits<double>::infinity());
    std::vector<Index> near(n, 0);
    best[0] = -1.0;
    Edge longest = noEdge();
    Index current = 0;
    for (Index added = 1; added < n; ++added) {
        const DeviceComplex p = points[current];
        Edge chosen = noEdge();
        for (Index j = 0; j < n; ++j) {
            bool closer = false;
            const double distance = squaredDistance(points[j], p);
            const Edge candidate = nearestOf(best[j], distance, j, closer);
            if (closer) {
                best[j] = distance;
                near[j] = current;
            }
            if (shorter(candidate, chosen)) {
                chosen = candidate;
            }
        }
        chosen.from = near[chosen.to];
        if (longest.to == kNoPoint || chosen.length > longest.length) {
            longest = chosen;
        }
        best[chosen.to] = -1.0;
        current = chosen.to;
    }
    Edge widest = noEdge();
    for (Index i = 0; i + 1 < n; ++i) {
        for (Index j = i + 1; j < n; ++j) {
            const Edge candidate{squaredDistance(points[j], points[i]), j, i};
            if (farther(candidate, widest)) {
                widest = candidate;
            }
        }
    }
    return SpanPairs{points[longest.from], points[longest.to], points[widest.from], points[widest.to]};
}

struct Extremes
{
    double longest = 0.0;
    double diameter = 0.0;
};

Extremes extremesOf(const SpanPairs & pairs)
{
    const auto onProcessor = [](DeviceComplex z) { return std::complex<double>(z.real(), z.imag()); };
    Extremes extremes;
    extremes.longest = std::abs(onProcessor(pairs.longestTo) - onProcessor(pairs.longestFrom));
    extremes.diameter = std::abs(onProcessor(pairs.farthestTo) - onProcessor(pairs.farthestFrom));
    return extremes;
}

std::vector<EpsilonProposal> startedProposals(const std::vector<Extremes> & extremes, bool walked)
{
    std::vector<EpsilonProposal> proposals(extremes.size());
    for (std::size_t f = 0; f < extremes.size(); ++f) {
        if (extremes[f].longest > 0.0 && extremes[f].diameter > 0.0) {
            EpsilonProposal & proposal = proposals[f];
            proposal.connected = extremes[f].longest;
            proposal.diameter = extremes[f].diameter;
            proposal.epsilon = walked ? extremes[f].longest : roundedUpToThreeFigures(extremes[f].longest);
            proposal.closes = !walked;
        }
    }
    return proposals;
}

struct Request
{
    std::size_t frequency;
    double epsilon;
};

bool closes(const WalkedContour & walk)
{
    return !walk.contour.empty() && !walk.truncated && !walk.fellBack;
}

struct Climber
{
    Climber(std::size_t f, const Extremes & extremes)
        : frequency(f), climb(extremes.longest, extremes.diameter), diameter(extremes.diameter)
    {
    }

    std::size_t frequency;
    EpsilonClimb climb;
    double diameter;
    std::size_t width = 1;
    bool exhausted = false;
    std::optional<double> closing;
    bool speculative = false;
    std::vector<double> candidates;
    std::vector<double> rungs;
};

void nextRound(Climber & c, std::vector<Request> & requests)
{
    c.candidates.clear();
    c.rungs.clear();
    c.speculative = false;
    if (c.closing.has_value()) {
        c.rungs = rungCandidates(*c.closing, c.diameter);
    } else {
        while (c.candidates.size() < c.width && !c.exhausted) {
            const std::optional<double> candidate = c.climb.next();
            if (candidate.has_value()) {
                c.candidates.push_back(*candidate);
            } else {
                c.exhausted = true;
            }
        }
        if (c.width == 1 && !c.candidates.empty()) {
            c.rungs = rungCandidates(c.candidates.front(), c.diameter);
            c.speculative = true;
        }
    }
    for (const double epsilon : c.candidates) {
        requests.push_back({c.frequency, epsilon});
    }
    for (const double epsilon : c.rungs) {
        requests.push_back({c.frequency, epsilon});
    }
}

double firstWithoutRetracing(const Climber & c, const WalkedContour * rungWalks, double closing)
{
    for (std::size_t k = 0; k < c.rungs.size(); ++k) {
        if (closes(rungWalks[k]) && !TemplateEngine::retraces(rungWalks[k].contour)) {
            return c.rungs[k];
        }
    }
    return closing;
}

bool decided(Climber & c, const WalkedContour * walks, EpsilonProposal & proposal)
{
    const WalkedContour * rungWalks = walks + c.candidates.size();
    if (c.closing.has_value()) {
        proposal.epsilon = firstWithoutRetracing(c, rungWalks, *c.closing);
        return true;
    }
    std::size_t k = 0;
    while (k < c.candidates.size() && !closes(walks[k])) {
        ++k;
    }
    if (k == c.candidates.size()) {
        c.width = std::min(2 * c.width, kWidestClimb);
        return c.exhausted;
    }
    proposal.closes = true;
    const double candidate = c.candidates[k];
    if (!TemplateEngine::retraces(walks[k].contour)) {
        proposal.epsilon = candidate;
        return true;
    }
    if (k == 0 && c.speculative) {
        proposal.epsilon = firstWithoutRetracing(c, rungWalks, candidate);
        return true;
    }
    c.closing = candidate;
    return false;
}

template <typename Walk>
bool climbed(std::vector<EpsilonProposal> & proposals, const std::vector<Extremes> & extremes, Walk walk,
             std::size_t & rounds)
{
    std::vector<Climber> open;
    for (std::size_t f = 0; f < proposals.size(); ++f) {
        if (proposals[f].diameter > 0.0) {
            open.emplace_back(f, extremes[f]);
        }
    }
    while (!open.empty()) {
        std::vector<Request> requests;
        for (Climber & c : open) {
            nextRound(c, requests);
        }
        const std::optional<std::vector<WalkedContour>> walks = walk(requests);
        if (!walks.has_value()) {
            return false;
        }
        ++rounds;
        std::vector<Climber> still;
        std::size_t at = 0;
        for (Climber & c : open) {
            const WalkedContour * mine = walks->data() + at;
            at += c.candidates.size() + c.rungs.size();
            if (!decided(c, mine, proposals[c.frequency])) {
                still.push_back(std::move(c));
            }
        }
        open = std::move(still);
    }
    return true;
}

unsigned int spanBlocks(std::size_t frequencies)
{
    int perMultiprocessor = 0;
    check(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&perMultiprocessor, spanningPairs, kSpanThreads, 0),
          "cudaOccupancyMaxActiveBlocksPerMultiprocessor");
    const std::size_t resident = static_cast<std::size_t>(std::max(perMultiprocessor, 1))
            * static_cast<std::size_t>(std::max(probe().multiprocessors, 1));
    return static_cast<unsigned int>(std::max<std::size_t>(1, std::min(resident, frequencies)));
}

std::optional<std::vector<Extremes>> extremesOnCard(const DeviceClouds & clouds, HullMetric metric,
                                                    double dbPerDegree, const CancellationToken * cancellation,
                                                    ProposedOnCard & result)
{
    const std::size_t frequencyCount = clouds.frequencies();
    const std::size_t plants = clouds.plants();
    const Stream & stream = clouds.stream();
    const cudaStream_t s = stream.get();
    const std::size_t batch = batchFrequencies(frequencyCount, plants, Work::kBytesPerPoint + kSpanBytesPerPoint);
    const std::size_t points = batch * plants;
    Work work(points, batch, metric == HullMetric::Nichols, stream);
    DeviceBuffer<double> reach(points, stream);
    DeviceBuffer<Index> nearest(points, stream);
    DeviceBuffer<SpanPairs> pairs(batch, stream);

    std::vector<Extremes> extremes(frequencyCount);
    for (std::size_t first = 0; first < frequencyCount; first += batch) {
        const Index count = static_cast<Index>(std::min(batch, frequencyCount - first));

        const auto computing = std::chrono::steady_clock::now();
        sortedBatch(work, clouds.values() + first * plants, static_cast<Index>(plants), count, metric, dbPerDegree, s);
        spanningPairs<<<spanBlocks(count), kSpanThreads, 0, s>>>(work.walk.data(), work.offsets.data(), count,
                                                                 reach.data(), nearest.data(), pairs.data());
        checkLaunch("the kernel of the spanning trees");
        if (!stream.finish(cancellation, nullptr)) {
            return std::nullopt;
        }
        result.kernelMilliseconds += millisecondsSince(computing);

        const auto copying = std::chrono::steady_clock::now();
        std::vector<SpanPairs> found(count);
        check(cudaMemcpyAsync(found.data(), pairs.data(), count * sizeof(SpanPairs), cudaMemcpyDeviceToHost, s),
              "cudaMemcpyAsync of the spanning trees");
        stream.synchronize();
        for (Index f = 0; f < count; ++f) {
            extremes[first + f] = extremesOf(found[f]);
        }
        result.copyMilliseconds += millisecondsSince(copying);
        ++result.batches;
    }
    return extremes;
}

std::optional<std::vector<WalkedContour>> walkedOnCopies(const DeviceClouds & clouds,
                                                         const std::shared_ptr<Stream> & stream,
                                                         const std::vector<Request> & requests, HullMetric metric,
                                                         double dbPerDegree, const CancellationToken * cancellation,
                                                         ProposedOnCard & result)
{
    const std::size_t plants = clouds.plants();
    std::size_t freeBytes = 0;
    std::size_t totalBytes = 0;
    check(cudaMemGetInfo(&freeBytes, &totalBytes), "cudaMemGetInfo");
    const std::size_t chunk = std::max<std::size_t>(
            1, std::min({requests.size(), freeBytes / 4 / (plants * sizeof(DeviceComplex)), std::size_t{65535}}));

    std::vector<WalkedContour> walks;
    walks.reserve(requests.size());
    for (std::size_t first = 0; first < requests.size(); first += chunk) {
        const std::size_t count = std::min(chunk, requests.size() - first);
        std::vector<Index> sources(count);
        std::vector<double> epsilon(count);
        for (std::size_t k = 0; k < count; ++k) {
            sources[k] = static_cast<Index>(requests[first + k].frequency);
            epsilon[k] = requests[first + k].epsilon;
        }

        const auto copying = std::chrono::steady_clock::now();
        DeviceBuffer<Index> sourcesOnCard = uploaded(sources, *stream);
        DeviceBuffer<DeviceComplex> copies(count * plants, *stream);
        copiedClouds<<<blocks(count * plants), kThreads, 0, stream->get()>>>(
                clouds.values(), sourcesOnCard.data(), plants, count * plants, copies.data());
        checkLaunch("the kernel of the copies of the clouds");
        stream->synchronize();
        result.kernelMilliseconds += millisecondsSince(copying);

        const DeviceClouds copied(stream, std::move(copies), count, plants);
        WalkedOnCard walked = walkedOnCard(copied, epsilon, metric, dbPerDegree, cancellation);
        result.kernelMilliseconds += walked.kernelMilliseconds;
        result.copyMilliseconds += walked.copyMilliseconds;
        result.batches += walked.batches;
        if (walked.cancelled) {
            return std::nullopt;
        }
        for (WalkedContour & walk : walked.walks) {
            walks.push_back(std::move(walk));
        }
    }
    result.walks += requests.size();
    return walks;
}

}

ProposedOnCard proposedOnCard(const DeviceClouds & clouds, HullMetric metric, double dbPerDegree, bool walked,
                              const CancellationToken * cancellation)
{
    ProposedOnCard result;
    const std::size_t frequencyCount = clouds.frequencies();
    result.proposals.assign(frequencyCount, {});
    if (frequencyCount == 0 || clouds.plants() == 0) {
        return result;
    }

    useProbedDevice();
    const std::optional<std::vector<Extremes>> extremes = extremesOnCard(clouds, metric, dbPerDegree, cancellation,
                                                                         result);
    if (!extremes.has_value()) {
        result.cancelled = true;
        return result;
    }
    result.proposals = startedProposals(*extremes, walked);
    if (walked) {
        const auto stream = std::make_shared<Stream>();
        const auto walk = [&](const std::vector<Request> & requests) {
            return walkedOnCopies(clouds, stream, requests, metric, dbPerDegree, cancellation, result);
        };
        result.cancelled = !climbed(result.proposals, *extremes, walk, result.rounds);
    }
    return result;
}

std::vector<EpsilonProposal> proposedOnProcessor(const CloudSet & clouds, HullMetric metric, double dbPerDegree,
                                                 bool walked)
{
    std::vector<Extremes> extremes;
    extremes.reserve(clouds.size());
    for (const ComplexCloud & cloud : clouds) {
        extremes.push_back(extremesOf(hostPairs(hostSorted(cloud, metric, dbPerDegree).sorted.walk)));
    }
    std::vector<EpsilonProposal> proposals = startedProposals(extremes, walked);
    if (walked) {
        const auto walk = [&](const std::vector<Request> & requests) {
            CloudSet copies;
            std::vector<double> epsilon;
            for (const Request & request : requests) {
                copies.push_back(clouds[request.frequency]);
                epsilon.push_back(request.epsilon);
            }
            return std::optional<std::vector<WalkedContour>>(walkedOnProcessor(copies, epsilon, metric, dbPerDegree));
        };
        std::size_t rounds = 0;
        climbed(proposals, extremes, walk, rounds);
    }
    return proposals;
}

}
