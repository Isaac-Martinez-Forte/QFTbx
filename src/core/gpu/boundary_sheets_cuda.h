#ifndef QFTBX_BOUNDARY_SHEETS_CUDA_H
#define QFTBX_BOUNDARY_SHEETS_CUDA_H

//Plain C++ interface to the CUDA boundary-sheet kernel: consumers compile
//without the CUDA toolkit; only the .cu implementation needs nvcc.

#include <complex>
#include <vector>

namespace qftbx {

/**
 * @brief The five boundary sheets of one design frequency, computed on the
 * GPU, in dB, laid out as [phase * magnitudeCount + magnitude].
 *
 * tracking carries the spread max|T| - min|T| in dB, the height the
 * tracking boundary is cut at (same contract as the CPU sheets).
 */
struct BoundarySheetsCuda {
    int phaseCount = 0;
    int magnitudeCount = 0;
    std::vector<float> stabilityNoise;
    std::vector<float> tracking;
    std::vector<float> outputDisturbance;
    std::vector<float> inputDisturbance;
    std::vector<float> controlEffort;
};

/**
 * @brief Computes the boundary sheets of one design frequency on the GPU.
 *
 * One thread per phase column, as designed in the TFM (Martinez Forte 2014,
 * section 4.5.2): a 2D cell mapping was evaluated there and discarded for
 * register/memory pressure; revisiting that choice requires measuring.
 * Values are computed in float (the TFM's deliberate precision/speed
 * trade-off). Throws qftbx::ComputationError on any CUDA failure.
 */
BoundarySheetsCuda boundarySheetsCuda(const std::vector<std::complex<double>> & valueSet,
                                      std::complex<double> nominal,
                                      const std::vector<float> & phases,
                                      const std::vector<float> & magnitudes);

} // namespace qftbx

#endif // QFTBX_BOUNDARY_SHEETS_CUDA_H
