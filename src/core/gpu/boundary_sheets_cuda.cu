//GPU computation of the boundary sheets (TFM: Martinez Forte 2014,
//"Paralelizacion de algoritmos QFT mediante OpenMP y CUDA", section 4.5.2).
//
//Design, as documented there: one thread per PHASE column, sweeping the
//magnitudes and the template inside the thread. A 2D cell mapping was
//evaluated in the TFM and discarded for register/memory pressure;
//revisiting that decision requires measuring on a real GPU.

#include "src/core/gpu/boundary_sheets_cuda.h"

#include <cmath>
#include <limits>

#include <cuda_runtime.h>

#include "src/core/gpu/cuda_utility.cuh"

namespace qftbx {

namespace {

__global__ void boundarySheetsKernel(const float * phases, const float * magnitudes,
                                     int magnitudeCount, int phaseCount,
                                     const CudaComplex * valueSet, CudaComplex nominal,
                                     int valueCount,
                                     float * stabilityNoise, float * tracking,
                                     float * outputDisturbance, float * inputDisturbance,
                                     float * controlEffort)
{
    const int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i >= phaseCount) {
        return;
    }

    for (int m = 0; m < magnitudeCount; m++) {

        //Nichols (dB, degrees) to the complex grid point L.
        CudaComplex L;
        L.x = pow(10., magnitudes[m] / 20.) * cos(phases[i] * M_PI / 180.);
        L.y = pow(10., magnitudes[m] / 20.) * sin(phases[i] * M_PI / 180.);

        //Worst case over the value set; the extremes start at IEEE
        //infinities (the original seeded them with the user's "infinity"
        //export value, corrupting the sweep when it was finite).
        float dStabilityNoise = -INFINITY;
        float dTrackingMin = INFINITY;
        float dOutputDisturbance = -INFINITY;
        float dInputDisturbance = -INFINITY;
        float dControlEffort = -INFINITY;

        for (int k = 0; k < valueCount; k++) {

            const CudaComplex denominator = (nominal / valueSet[k]) + L;

            const float dStabilityNoiseCandidate =
                20 * log10(cudaAbs(L / denominator));
            const float dOutputDisturbanceCandidate =
                20 * log10(cudaAbs((nominal / valueSet[k]) / denominator));
            const float dInputDisturbanceCandidate =
                20 * log10(cudaAbs(nominal / denominator));
            const float dControlEffortCandidate =
                20 * log10(cudaAbs((L / valueSet[k]) / denominator));

            //Independent updates: the original chained the min behind an
            //"else", so the tracking minimum was skipped whenever the
            //maximum advanced and the tracking sheet came out wrong.
            if (dStabilityNoiseCandidate > dStabilityNoise) {
                dStabilityNoise = dStabilityNoiseCandidate;
            }
            if (dStabilityNoiseCandidate < dTrackingMin) {
                dTrackingMin = dStabilityNoiseCandidate;
            }
            if (dOutputDisturbanceCandidate > dOutputDisturbance) {
                dOutputDisturbance = dOutputDisturbanceCandidate;
            }
            if (dInputDisturbanceCandidate > dInputDisturbance) {
                dInputDisturbance = dInputDisturbanceCandidate;
            }
            if (dControlEffortCandidate > dControlEffort) {
                dControlEffort = dControlEffortCandidate;
            }
        }

        const int cell = i * magnitudeCount + m;
        stabilityNoise[cell] = dStabilityNoise;
        //The spread max - min in dB: the height the tracking boundary is
        //cut at (the original stored the bare minimum, which never matched
        //the threshold the consumer cuts with).
        tracking[cell] = dStabilityNoise - dTrackingMin;
        outputDisturbance[cell] = dOutputDisturbance;
        inputDisturbance[cell] = dInputDisturbance;
        controlEffort[cell] = dControlEffort;
    }
}

} // namespace

BoundarySheetsCuda boundarySheetsCuda(const std::vector<std::complex<double>> & valueSet,
                                      std::complex<double> nominal,
                                      const std::vector<float> & phases,
                                      const std::vector<float> & magnitudes)
{
    const int valueCount = static_cast<int>(valueSet.size());
    const int phaseCount = static_cast<int>(phases.size());
    const int magnitudeCount = static_cast<int>(magnitudes.size());
    const std::size_t cellCount =
        static_cast<std::size_t>(phaseCount) * static_cast<std::size_t>(magnitudeCount);

    //The value set travels as float2 (the TFM's precision/speed choice).
    std::vector<CudaComplex> hostValues(valueCount);
    for (int i = 0; i < valueCount; i++) {
        hostValues[i].x = static_cast<float>(valueSet[i].real());
        hostValues[i].y = static_cast<float>(valueSet[i].imag());
    }

    DeviceBuffer<CudaComplex> deviceValues(valueCount);
    deviceValues.copyFromHost(hostValues.data());

    DeviceBuffer<float> devicePhases(phaseCount);
    devicePhases.copyFromHost(phases.data());

    DeviceBuffer<float> deviceMagnitudes(magnitudeCount);
    deviceMagnitudes.copyFromHost(magnitudes.data());

    DeviceBuffer<float> deviceStabilityNoise(cellCount);
    DeviceBuffer<float> deviceTracking(cellCount);
    DeviceBuffer<float> deviceOutputDisturbance(cellCount);
    DeviceBuffer<float> deviceInputDisturbance(cellCount);
    DeviceBuffer<float> deviceControlEffort(cellCount);

    CudaComplex deviceNominal;
    deviceNominal.x = static_cast<float>(nominal.real());
    deviceNominal.y = static_cast<float>(nominal.imag());

    const int threadsPerBlock = 128;
    const int blocksPerGrid = (phaseCount + threadsPerBlock - 1) / threadsPerBlock;

    boundarySheetsKernel<<<blocksPerGrid, threadsPerBlock>>>(
        devicePhases.data(), deviceMagnitudes.data(), magnitudeCount, phaseCount,
        deviceValues.data(), deviceNominal, valueCount,
        deviceStabilityNoise.data(), deviceTracking.data(),
        deviceOutputDisturbance.data(), deviceInputDisturbance.data(),
        deviceControlEffort.data());
    cudaCheckKernel("boundarySheetsKernel");

    BoundarySheetsCuda sheets;
    sheets.phaseCount = phaseCount;
    sheets.magnitudeCount = magnitudeCount;
    sheets.stabilityNoise.resize(cellCount);
    sheets.tracking.resize(cellCount);
    sheets.outputDisturbance.resize(cellCount);
    sheets.inputDisturbance.resize(cellCount);
    sheets.controlEffort.resize(cellCount);

    deviceStabilityNoise.copyToHost(sheets.stabilityNoise.data());
    deviceTracking.copyToHost(sheets.tracking.data());
    deviceOutputDisturbance.copyToHost(sheets.outputDisturbance.data());
    deviceInputDisturbance.copyToHost(sheets.inputDisturbance.data());
    deviceControlEffort.copyToHost(sheets.controlEffort.data());

    return sheets;
}

} // namespace qftbx
