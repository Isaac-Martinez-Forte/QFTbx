/**
 * @file
 * @brief The complex number of the card.
 *
 * libcu++'s, laid out as std::complex<double>, two doubles, so values cross
 * between the two with a plain copy.
 */

#ifndef QFTBX_GPU_DEVICE_COMPLEX_CUH
#define QFTBX_GPU_DEVICE_COMPLEX_CUH

#include <complex>

#include <cuda/std/complex>

namespace qftbx::gpu {

using DeviceComplex = cuda::std::complex<double>;

static_assert(sizeof(DeviceComplex) == sizeof(std::complex<double>), "the two complex types share a layout");

}

#endif
