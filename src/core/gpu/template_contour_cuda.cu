//GPU epsilon-hull (TFM: Martinez Forte 2014, "Paralelizacion de algoritmos
//QFT mediante OpenMP y CUDA", section 4.5.1).
//
//Design, as documented there: only the O(n) candidate searches run on the
//GPU - a map kernel computing every candidate's angle followed by a Thrust
//min-element reduction - with the point cloud copied to the device once
//and reused across the whole walk; the walk itself stays on the host.
//
//This is the historical RELAXED walk (parity reference:
//TemplateEngine::epsilonHullRelaxed): start at the largest imaginary part,
//the previous point excluded as candidate, deduplicated output, silent
//truncation at MAXP. The faithful EPSHULL.M walk with its documented
//fallback is CPU-only for now (phase-8 note in REFACTOR_PLAN).

#include "src/core/gpu/template_contour_cuda.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <unordered_set>

#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/extrema.h>

#include "src/core/gpu/cuda_utility.cuh"

namespace qftbx {

namespace {

//Each candidate carries its angle (the walk minimises it) and its distance
//to the current point: on equal angles the LARGER distance wins, the same
//tie-break the CPU implementation applies (the original GPU code dropped
//it and took the first minimum).
struct Candidate {
    double angle;
    double distance;
};

struct CandidateLess {
    __host__ __device__ bool operator()(const Candidate & a, const Candidate & b) const
    {
        if (a.angle != b.angle) {
            return a.angle < b.angle;
        }
        return a.distance > b.distance;
    }
};

__global__ void secondPointKernel(CudaComplex first, const CudaComplex * points,
                                  float epsilon, Candidate * result, int pointCount)
{
    const int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i >= pointCount) {
        return;
    }

    const double distance = cudaAbs(points[i] - first);

    if (distance > 0 && distance <= epsilon) {

        double angle = cudaArg(points[i] - first);

        if (angle < 0) {
            angle += 2 * M_PI;
        }

        result[i].angle = angle - acos(distance / epsilon);
        result[i].distance = distance;
    } else {
        result[i].angle = INFINITY;
        result[i].distance = 0;
    }
}

__global__ void nextPointKernel(CudaComplex previous, CudaComplex current,
                                const CudaComplex * points, float epsilon,
                                Candidate * result, int pointCount)
{
    const int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i >= pointCount) {
        return;
    }

    const double distance = cudaAbs(points[i] - current);

    //Relaxed-walk candidates: within (0, epsilon] of the current point and
    //neither the current nor the previous point.
    if (distance > 0 && distance <= epsilon &&
            points[i] != previous && points[i] != current) {

        double phase = cudaArg((points[i] - current) / (previous - current));

        if (phase < 0) {
            phase += 2 * M_PI;
        }

        const double aco1 = acos(distance / epsilon);
        const double aco2 = acos(cudaAbs(previous - current) / epsilon);

        //psi has three cases, as in EPSHULL.M.
        double psi;
        if (phase == 0) {
            psi = 2 * M_PI - aco1 - aco2;
        } else if (phase < aco2) {
            psi = phase + aco1 - aco2;
        } else {
            psi = phase - aco1 - aco2;
        }

        if (psi < 0) {
            psi += 2 * M_PI;
        }

        result[i].angle = psi;
        result[i].distance = distance;
    } else {
        result[i].angle = INFINITY;
        result[i].distance = 0;
    }
}

//Runs one search kernel already enqueued and reduces its result. Returns
//the winning index, or -1 when no candidate exists.
int reduceCandidates(DeviceBuffer<Candidate> & candidates)
{
    cudaCheckKernel("epsilon-hull search kernel");

    thrust::device_ptr<Candidate> first(candidates.data());
    thrust::device_ptr<Candidate> last = first + candidates.count();
    thrust::device_ptr<Candidate> best =
        thrust::min_element(first, last, CandidateLess());

    const Candidate winner = *best;

    if (winner.angle == std::numeric_limits<double>::infinity()) {
        return -1;
    }

    return static_cast<int>(best - first);
}

} // namespace

std::vector<std::complex<double>> epsilonHullCuda(const std::vector<std::complex<double>> & points,
                                                  float epsilon)
{
    const int pointCount = static_cast<int>(points.size());

    if (pointCount == 0) {
        return {};
    }

    //The cloud travels as float2 (the TFM's precision/speed choice) and is
    //copied to the device ONCE for the whole walk. The returned values are
    //the original doubles, selected by index.
    std::vector<CudaComplex> hostPoints(pointCount);

    //First point: the largest imaginary part (relaxed-walk convention;
    //the faithful EPSHULL.M walk starts at the largest real part).
    int firstIndex = 0;
    for (int i = 0; i < pointCount; i++) {
        hostPoints[i].x = static_cast<float>(points[i].real());
        hostPoints[i].y = static_cast<float>(points[i].imag());

        if (hostPoints[i].y > hostPoints[firstIndex].y) {
            firstIndex = i;
        }
    }

    DeviceBuffer<CudaComplex> devicePoints(pointCount);
    devicePoints.copyFromHost(hostPoints.data());

    DeviceBuffer<Candidate> candidates(pointCount);

    const int threadsPerBlock = 128;
    const int blocksPerGrid = (pointCount + threadsPerBlock - 1) / threadsPerBlock;

    //-------------------------------------------------- second point
    secondPointKernel<<<blocksPerGrid, threadsPerBlock>>>(
        hostPoints[firstIndex], devicePoints.data(), epsilon,
        candidates.data(), pointCount);

    const int secondIndex = reduceCandidates(candidates);

    if (secondIndex < 0) {
        return {};
    }

    //-------------------------------------------------- the walk
    std::vector<int> walk;
    walk.push_back(firstIndex);
    walk.push_back(secondIndex);

    int previousIndex = firstIndex;
    int currentIndex = secondIndex;

    nextPointKernel<<<blocksPerGrid, threadsPerBlock>>>(
        hostPoints[previousIndex], hostPoints[currentIndex], devicePoints.data(),
        epsilon, candidates.data(), pointCount);
    int nextIndex = reduceCandidates(candidates);

    if (nextIndex < 0) {
        return {};
    }

    //Same bound as the CPU relaxed walk (the original truncated at 1x).
    const std::size_t MAXP = 3 * static_cast<std::size_t>(pointCount);

    //Stops when the walk returns to the initial (first, second) pair.
    while (firstIndex != currentIndex || secondIndex != nextIndex) {

        walk.push_back(nextIndex);

        if (walk.size() > MAXP) {
            break; //silent truncation: partial contour (historical behaviour).
        }

        previousIndex = currentIndex;
        currentIndex = nextIndex;

        nextPointKernel<<<blocksPerGrid, threadsPerBlock>>>(
            hostPoints[previousIndex], hostPoints[currentIndex], devicePoints.data(),
            epsilon, candidates.data(), pointCount);
        nextIndex = reduceCandidates(candidates);

        if (nextIndex < 0) {
            return {};
        }
    }

    //Output deduplication preserving order (historical behaviour; the
    //original compared against the wrong vector and could leave
    //duplicates in).
    std::vector<std::complex<double>> contour;
    contour.reserve(walk.size());
    std::unordered_set<int> seen;

    for (const int index : walk) {
        if (seen.insert(index).second) {
            contour.push_back(points[index]);
        }
    }

    return contour;
}

} // namespace qftbx
