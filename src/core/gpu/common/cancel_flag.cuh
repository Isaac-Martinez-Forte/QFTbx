/**
 * @file
 * @brief A flag the host raises and the kernels read while they run.
 *
 * It lives in pinned host memory mapped into the card's address space, so a
 * kernel sees it change without a copy; both sides read and write it
 * through libcu++'s system-scope atomics, with relaxed ordering, since it
 * carries no data, only permission to stop. cancelRequested() is what a
 * long kernel polls between pieces of work.
 */

#ifndef QFTBX_GPU_CANCEL_FLAG_CUH
#define QFTBX_GPU_CANCEL_FLAG_CUH

#include <cuda/atomic>
#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"

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

}

#endif
