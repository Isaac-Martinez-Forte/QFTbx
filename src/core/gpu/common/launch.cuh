/**
 * @file
 * @brief The size of a launch, from the kernel and the card.
 *
 * blockSizeFor() asks the occupancy API for the block that keeps the card
 * fullest with that kernel, so nothing is tuned to one card; blocksFor() is
 * the number of such blocks that covers n threads.
 */

#ifndef QFTBX_GPU_LAUNCH_CUH
#define QFTBX_GPU_LAUNCH_CUH

#include <cstddef>

#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"

namespace qftbx::gpu {

template <typename Kernel>
int blockSizeFor(Kernel kernel, std::size_t dynamicSharedBytes = 0)
{
    int minimumGrid = 0;
    int block = 0;
    check(cudaOccupancyMaxPotentialBlockSize(&minimumGrid, &block, kernel, dynamicSharedBytes, 0),
          "cudaOccupancyMaxPotentialBlockSize");
    return block;
}

inline unsigned int blocksFor(std::size_t threads, int block)
{
    return static_cast<unsigned int>((threads + static_cast<std::size_t>(block) - 1) / static_cast<std::size_t>(block));
}

}

#endif
