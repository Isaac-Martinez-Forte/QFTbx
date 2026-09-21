/**
 * @file
 * @brief Plain C++ interface to the CUDA epsilon-hull of a template.
 *
 * Declares the function that walks the epsilon-hull contour of a point
 * cloud with the candidate searches on the GPU. Consumers compile without
 * the CUDA toolkit; only the .cu implementation needs nvcc. Following the
 * master's thesis (Martínez Forte 2014, section 4.5.1), the cloud is copied
 * to the device once as floats and only the linear candidate searches run
 * there, a map kernel and a reduction per step, while the walk stays on the
 * host and returns the original doubles selected by index. The walk is the
 * relaxed one: it starts at the largest imaginary part, excludes the
 * previous point as a candidate and deduplicates its output.
 */

#ifndef QFTBX_TEMPLATE_CONTOUR_CUDA_H
#define QFTBX_TEMPLATE_CONTOUR_CUDA_H

/// Plain C++ interface to the CUDA epsilon-hull: consumers compile without
/// the CUDA toolkit; only the .cu implementation needs nvcc.

#include <complex>
#include <vector>

namespace qftbx {

/**
 * @brief Computes the epsilon-hull contour of a template on the GPU.
 *
 * Implements the RELAXED walk (the parity reference is
 * TemplateEngine::epsilonHullRelaxed): start at the largest imaginary
 * part, previous point excluded as candidate, deduplicated output. The
 * walk faithful to EPSHULL.M, with its fallback, runs on the CPU only.
 *
 * Per the master's thesis (Martínez Forte 2014, section 4.5.1) only the O(n) candidate
 * searches run on the GPU (a map kernel plus a Thrust reduction), with the
 * point cloud copied to the device once and reused across the walk; the
 * walk itself stays on the host. Points are stored as float on the device
 * (its precision/speed trade-off); the returned values are the
 * original doubles selected by index.
 *
 * @return The contour points, empty when no contour exists (isolated
 * points at distance > epsilon). Throws qftbx::ComputationError on any
 * CUDA failure.
 */
std::vector<std::complex<double>> epsilonHullCuda(const std::vector<std::complex<double>> & points,
                                                  float epsilon);

}

#endif
