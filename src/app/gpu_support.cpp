/**
 * @file
 * @brief Installs the CUDA provider in a build with CUDA.
 */

#include "src/app/gpu_support.h"

#include "src/core/pipeline/gpu_provider.h"

#ifdef QFTBX_WITH_CUDA
#include "src/core/gpu/common/cuda_provider.h"
#endif

namespace qftbx {

void installGpuSupport()
{
#ifdef QFTBX_WITH_CUDA
    if (gpuProvider() == nullptr) {
        installGpuProvider(gpu::makeCudaProvider());
    }
#endif
}

}
