/**
 * @file
 * @brief A flag the host raises and the kernels read while they run.
 *
 * It lives in pinned host memory mapped into the card's address space, so a
 * kernel sees it change without a copy; both sides read and write it
 * through libcu++'s system-scope atomics, with relaxed ordering, since it
 * carries no data, only permission to stop. cancelRequested() is what a
 * long kernel polls between pieces of work. A warp or a block asks with one
 * thread and shares the answer (warpCancelRequested, blockCancelRequested),
 * so that its threads stop together; cancelAsked() is the question in code
 * the processor runs too, where it is never asked.
 */

#ifndef QFTBX_GPU_CANCEL_FLAG_CUH
#define QFTBX_GPU_CANCEL_FLAG_CUH

#include <cuda/atomic>
#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/host_device.h"

namespace qftbx::gpu {

class CancelFlag
{
public:
    CancelFlag()
    {
        check(cudaHostAlloc(reinterpret_cast<void **>(&m_host), sizeof(int), cudaHostAllocMapped), "cudaHostAlloc");
        *m_host = 0;
        check(cudaHostGetDevicePointer(reinterpret_cast<void **>(&m_device), m_host, 0), "cudaHostGetDevicePointer");
    }

    ~CancelFlag()
    {
        cudaFreeHost(m_host);
    }

    CancelFlag(const CancelFlag &) = delete;
    CancelFlag & operator=(const CancelFlag &) = delete;

    void raise()
    {
        cuda::atomic_ref<int, cuda::thread_scope_system>(*m_host).store(1, cuda::memory_order_relaxed);
    }

    void clear()
    {
        cuda::atomic_ref<int, cuda::thread_scope_system>(*m_host).store(0, cuda::memory_order_relaxed);
    }

    int * device() const { return m_device; }

private:
    int * m_host = nullptr;
    int * m_device = nullptr;
};

__device__ inline bool cancelRequested(int * flag)
{
    return flag != nullptr
            && cuda::atomic_ref<int, cuda::thread_scope_system>(*flag).load(cuda::memory_order_relaxed) != 0;
}

__device__ inline bool warpCancelRequested(int * flag)
{
    int asked = 0;
    if (threadIdx.x % 32 == 0) {
        asked = cancelRequested(flag) ? 1 : 0;
    }
    return __shfl_sync(0xffffffffu, asked, 0) != 0;
}

__device__ inline bool blockCancelRequested(int * flag, int & shared)
{
    __syncthreads();
    if (threadIdx.x == 0) {
        shared = cancelRequested(flag) ? 1 : 0;
    }
    __syncthreads();
    return shared != 0;
}

QFTBX_HD inline bool cancelAsked(int * flag)
{
#ifdef __CUDA_ARCH__
    return cancelRequested(flag);
#else
    (void) flag;
    return false;
#endif
}

}

#endif
