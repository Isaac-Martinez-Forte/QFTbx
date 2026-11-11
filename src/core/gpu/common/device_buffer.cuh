/**
 * @file
 * @brief Memory on the card and pinned memory on the host, owned.
 *
 * A DeviceBuffer is reserved and freed in the order of its stream, from the
 * card's memory pool where the card has pools and with a plain reservation
 * where it does not, so the stream must outlive its buffers. Copies are
 * asynchronous in the same stream; from and to a PinnedBuffer they overlap
 * with the work of other streams. A buffer of no elements reserves nothing.
 * uploaded() is a buffer with a copy of a vector.
 */

#ifndef QFTBX_GPU_DEVICE_BUFFER_CUH
#define QFTBX_GPU_DEVICE_BUFFER_CUH

#include <cstddef>
#include <utility>
#include <vector>

#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/stream.cuh"

namespace qftbx::gpu {

template <typename T>
class DeviceBuffer
{
public:
    DeviceBuffer() = default;

    DeviceBuffer(std::size_t count, const Stream & stream) : m_count(count), m_stream(stream.get())
    {
        if (count == 0) {
            return;
        }
        m_pooled = probe().memoryPools;
        void * memory = nullptr;
        if (m_pooled) {
            check(cudaMallocAsync(&memory, count * sizeof(T), m_stream), "cudaMallocAsync");
        } else {
            check(cudaMalloc(&memory, count * sizeof(T)), "cudaMalloc");
        }
        m_data = static_cast<T *>(memory);
    }

    ~DeviceBuffer()
    {
        release();
    }

    DeviceBuffer(const DeviceBuffer &) = delete;
    DeviceBuffer & operator=(const DeviceBuffer &) = delete;

    DeviceBuffer(DeviceBuffer && other) noexcept
        : m_data(std::exchange(other.m_data, nullptr)), m_count(std::exchange(other.m_count, 0)),
          m_stream(other.m_stream), m_pooled(other.m_pooled)
    {
    }

    DeviceBuffer & operator=(DeviceBuffer && other) noexcept
    {
        if (this != &other) {
            release();
            m_data = std::exchange(other.m_data, nullptr);
            m_count = std::exchange(other.m_count, 0);
            m_stream = other.m_stream;
            m_pooled = other.m_pooled;
        }
        return *this;
    }

    T * data() { return m_data; }
    const T * data() const { return m_data; }
    std::size_t size() const { return m_count; }

    void upload(const T * host)
    {
        if (m_count != 0) {
            check(cudaMemcpyAsync(m_data, host, m_count * sizeof(T), cudaMemcpyHostToDevice, m_stream),
                  "cudaMemcpyAsync to the card");
        }
    }

    void download(T * host) const
    {
        if (m_count != 0) {
            check(cudaMemcpyAsync(host, m_data, m_count * sizeof(T), cudaMemcpyDeviceToHost, m_stream),
                  "cudaMemcpyAsync from the card");
        }
    }

    void fill(unsigned char byte)
    {
        if (m_count != 0) {
            check(cudaMemsetAsync(m_data, byte, m_count * sizeof(T), m_stream), "cudaMemsetAsync");
        }
    }

private:
    void release() noexcept
    {
        if (m_data == nullptr) {
            return;
        }
        if (m_pooled) {
            cudaFreeAsync(m_data, m_stream);
        } else {
            cudaFree(m_data);
        }
        m_data = nullptr;
    }

    T * m_data = nullptr;
    std::size_t m_count = 0;
    cudaStream_t m_stream = nullptr;
    bool m_pooled = false;
};

template <typename T>
class PinnedBuffer
{
public:
    PinnedBuffer() = default;

    explicit PinnedBuffer(std::size_t count) : m_count(count)
    {
        if (count != 0) {
            check(cudaMallocHost(reinterpret_cast<void **>(&m_data), count * sizeof(T)), "cudaMallocHost");
        }
    }

    ~PinnedBuffer()
    {
        if (m_data != nullptr) {
            cudaFreeHost(m_data);
        }
    }

    PinnedBuffer(const PinnedBuffer &) = delete;
    PinnedBuffer & operator=(const PinnedBuffer &) = delete;

    PinnedBuffer(PinnedBuffer && other) noexcept
        : m_data(std::exchange(other.m_data, nullptr)), m_count(std::exchange(other.m_count, 0))
    {
    }

    PinnedBuffer & operator=(PinnedBuffer && other) noexcept
    {
        if (this != &other) {
            if (m_data != nullptr) {
                cudaFreeHost(m_data);
            }
            m_data = std::exchange(other.m_data, nullptr);
            m_count = std::exchange(other.m_count, 0);
        }
        return *this;
    }

    T * data() { return m_data; }
    const T * data() const { return m_data; }
    std::size_t size() const { return m_count; }
    T & operator[](std::size_t i) { return m_data[i]; }
    const T & operator[](std::size_t i) const { return m_data[i]; }

private:
    T * m_data = nullptr;
    std::size_t m_count = 0;
};

template <typename T>
DeviceBuffer<T> uploaded(const std::vector<T> & host, const Stream & stream)
{
    DeviceBuffer<T> buffer(host.size(), stream);
    if (!host.empty()) {
        buffer.upload(host.data());
    }
    return buffer;
}

}

#endif
