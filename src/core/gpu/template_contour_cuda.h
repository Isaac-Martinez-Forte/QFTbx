#ifndef QFTBX_TEMPLATE_CONTOUR_CUDA_H
#define QFTBX_TEMPLATE_CONTOUR_CUDA_H

//Plain C++ interface to the CUDA epsilon-hull: consumers compile without
//the CUDA toolkit; only the .cu implementation needs nvcc.

#include <complex>
#include <vector>

namespace qftbx {

/**
 * @brief Computes the epsilon-hull contour of a template on the GPU.
 *
 * Implements the historical RELAXED walk (the parity reference is
 * TemplateEngine::epsilonHullRelaxed): start at the largest imaginary
 * part, previous point excluded as candidate, deduplicated output. The
 * faithful EPSHULL.M walk with its documented fallback is CPU-only for
 * now (phase-8 note in REFACTOR_PLAN).
 *
 * Per the TFM (Martinez Forte 2014, section 4.5.1) only the O(n) candidate
 * searches run on the GPU (a map kernel plus a Thrust reduction), with the
 * point cloud copied to the device once and reused across the walk; the
 * walk itself stays on the host. Points are stored as float on the device
 * (the TFM's precision/speed trade-off); the returned values are the
 * original doubles selected by index.
 *
 * @return The contour points, empty when no contour exists (isolated
 * points at distance > epsilon). Throws qftbx::ComputationError on any
 * CUDA failure.
 */
std::vector<std::complex<double>> epsilonHullCuda(const std::vector<std::complex<double>> & points,
                                                  float epsilon);

} // namespace qftbx

#endif // QFTBX_TEMPLATE_CONTOUR_CUDA_H
