/**
 * @file
 * @brief A CUDA stream and an event, owned, and the wait that can be given
 * up on.
 *
 * Streams do not synchronise with the legacy default stream. finish()
 * waits for all the stream holds while it polls the cancellation token:
 * once the token is raised it raises the flag the kernels read, so a long
 * kernel stops early, still waits for the stream to drain, since its
 * buffers must outlive the work, and answers false. A failure on the card
 * throws CudaError either way.
 */

#ifndef QFTBX_GPU_STREAM_CUH
#define QFTBX_GPU_STREAM_CUH

#include <chrono>
#include <thread>

#include <cuda_runtime.h>

#include "src/core/gpu/common/cancel_flag.cuh"
#include "src/core/gpu/common/check.cuh"
#include "src/core/pipeline/cancellation.h"

namespace qftbx::gpu {

class Stream
{
public:
    Stream()
    {
        check(cudaStreamCreateWithFlags(&m_stream, cudaStreamNonBlocking), "cudaStreamCreateWithFlags");
    }

    ~Stream()
    {
        cudaStreamDestroy(m_stream);
    }

    Stream(const Stream &) = delete;
    Stream & operator=(const Stream &) = delete;

    cudaStream_t get() const { return m_stream; }

    void synchronize() const
    {
        check(cudaStreamSynchronize(m_stream), "cudaStreamSynchronize");
    }

    bool finish(const CancellationToken * token, CancelFlag * flag) const
    {
        bool cancelled = false;
        while (true) {
            const cudaError_t status = cudaStreamQuery(m_stream);
            if (status == cudaSuccess) {
                return !cancelled;
            }
            if (status != cudaErrorNotReady) {
                check(status, "cudaStreamQuery");
            }
            if (!cancelled && cancellationAsked(token)) {
                cancelled = true;
                if (flag != nullptr) {
                    flag->raise();
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

private:
    cudaStream_t m_stream = nullptr;
};

class Event
{
public:
    Event()
    {
        check(cudaEventCreateWithFlags(&m_event, cudaEventDisableTiming), "cudaEventCreateWithFlags");
    }

    ~Event()
    {
        cudaEventDestroy(m_event);
    }

    Event(const Event &) = delete;
    Event & operator=(const Event &) = delete;

    void record(const Stream & stream)
    {
        check(cudaEventRecord(m_event, stream.get()), "cudaEventRecord");
    }

    void makeWait(const Stream & stream) const
    {
        check(cudaStreamWaitEvent(stream.get(), m_event, 0), "cudaStreamWaitEvent");
    }

    cudaEvent_t get() const { return m_event; }

private:
    cudaEvent_t m_event = nullptr;
};

}

#endif
