/**
 * @file
 * @brief Every CUDA call is checked, and a failure throws CudaError naming
 * the call.
 *
 * checkLaunch() reads the error a kernel launch leaves behind.
 */

#ifndef QFTBX_GPU_CHECK_CUH
#define QFTBX_GPU_CHECK_CUH

#include <string>

#include <cuda_runtime.h>

#include "src/core/gpu/common/cuda_error.h"

namespace qftbx::gpu {

inline void check(cudaError_t status, const char * what)
{
    if (status != cudaSuccess) {
        throw CudaError(std::string(what) + ": " + cudaGetErrorString(status), static_cast<int>(status));
    }
}

inline void checkLaunch(const char * what)
{
    check(cudaGetLastError(), what);
}

}

#endif
