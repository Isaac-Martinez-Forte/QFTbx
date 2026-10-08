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
 * relaxed one, whose CPU reference is TemplateEngine::epsilonHullRelaxed:
 * it starts at the largest imaginary part, excludes the previous point as
 * a candidate and deduplicates its output; the walk faithful to EPSHULL.M
 * runs on the CPU only. The contour is empty when there is none, the
 * points lying farther than epsilon apart, and any CUDA failure throws
 * ComputationError.
 */

#ifndef QFTBX_TEMPLATE_CONTOUR_CUDA_H
#define QFTBX_TEMPLATE_CONTOUR_CUDA_H

#include <complex>
#include <vector>

namespace qftbx {

std::vector<std::complex<double>> epsilonHullCuda(const std::vector<std::complex<double>> & points,
                                                  float epsilon);

}

#endif
