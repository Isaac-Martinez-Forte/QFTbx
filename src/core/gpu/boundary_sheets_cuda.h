/**
 * @file
 * @brief Plain C++ interface to the CUDA kernel of the boundary sheets.
 *
 * Declares the five sheets of one design frequency as the GPU returns them,
 * in dB and laid out phase-major, [phase * magnitudeCount + magnitude], and
 * the function that computes them. The tracking sheet carries the spread
 * max|T| - min|T| in dB, the height the tracking boundary is cut at, as
 * the CPU sheets do. It includes nothing from the CUDA toolkit, so its
 * consumers compile without it; only the .cu implementation needs nvcc.
 * The kernel runs one thread per phase column, which keeps the register and
 * memory pressure below that of one thread per cell (Martínez Forte 2014,
 * section 4.5.2), and computes in float. Any CUDA failure throws
 * qftbx::ComputationError.
 */

#ifndef QFTBX_BOUNDARY_SHEETS_CUDA_H
#define QFTBX_BOUNDARY_SHEETS_CUDA_H

#include <complex>
#include <vector>

namespace qftbx {

struct BoundarySheetsCuda {
    int phaseCount = 0;
    int magnitudeCount = 0;
    std::vector<float> stabilityNoise;
    std::vector<float> tracking;
    std::vector<float> outputDisturbance;
    std::vector<float> inputDisturbance;
    std::vector<float> controlEffort;
};

BoundarySheetsCuda boundarySheetsCuda(const std::vector<std::complex<double>> & valueSet,
                                      std::complex<double> nominal,
                                      const std::vector<float> & phases,
                                      const std::vector<float> & magnitudes);

}

#endif
