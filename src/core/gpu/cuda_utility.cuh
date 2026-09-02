#ifndef QFTBX_CUDA_UTILITY_CUH
#define QFTBX_CUDA_UTILITY_CUH

//Shared helpers of the CUDA kernels (.cu-only: this header needs nvcc).

#include <cstddef>
#include <string>

#include <cuda_runtime.h>

#include "Modelo/Herramientas/exception.h"

namespace qftbx {

//Every CUDA call goes through this check: failures used to be ignored (or
//worse, exit(EXIT_FAILURE) from inside a GUI application). The GUI already
//catches qftbx::Exception around every computation.
inline void cudaCheck(cudaError_t status, const char * what)
{
    if (status != cudaSuccess) {
        throw ComputationError(std::string("CUDA failure in ") + what + ": "
                               + cudaGetErrorString(status));
    }
}

inline void cudaCheckKernel(const char * what)
{
    cudaCheck(cudaGetLastError(), what);
}

/**
 * @brief RAII device buffer: the raw cudaMalloc/cudaFree pairs used to leak
 * on every early return.
 */
template <typename T>
class DeviceBuffer
{
public:
    explicit DeviceBuffer(std::size_t count) : m_count(count)
    {
        cudaCheck(cudaMalloc(reinterpret_cast<void **>(&m_data), count * sizeof(T)),
                  "cudaMalloc");
    }

    ~DeviceBuffer()
    {
        cudaFree(m_data);
    }

    DeviceBuffer(const DeviceBuffer &) = delete;
    DeviceBuffer & operator=(const DeviceBuffer &) = delete;

    void copyFromHost(const T * host)
    {
        cudaCheck(cudaMemcpy(m_data, host, m_count * sizeof(T), cudaMemcpyHostToDevice),
                  "cudaMemcpy host-to-device");
    }

    void copyToHost(T * host) const
    {
        cudaCheck(cudaMemcpy(host, m_data, m_count * sizeof(T), cudaMemcpyDeviceToHost),
                  "cudaMemcpy device-to-host");
    }

    T * data() { return m_data; }
    const T * data() const { return m_data; }
    std::size_t count() const { return m_count; }

private:
    T * m_data = nullptr;
    std::size_t m_count = 0;
};

//float2 complex helpers shared by the kernels. Same numerics as the
//original TFM code (float storage, double where it used double): behaviour
//is preserved; retuning the precision is a measured change for later.

typedef float2 CudaComplex;

static __device__ __host__ inline double cudaAbs(CudaComplex a)
{
    //hypot avoids the under/overflow the hand-rolled version guarded
    //against, without its division-by-zero when both parts are 0.
    return hypot(static_cast<double>(a.x), static_cast<double>(a.y));
}

static __device__ __host__ inline double cudaArg(CudaComplex a)
{
    return atan2(static_cast<double>(a.y), static_cast<double>(a.x));
}

static __device__ __host__ inline CudaComplex operator-(CudaComplex a, CudaComplex b)
{
    a.x = a.x - b.x;
    a.y = a.y - b.y;
    return a;
}

static __device__ __host__ inline CudaComplex operator+(CudaComplex a, CudaComplex b)
{
    a.x = a.x + b.x;
    a.y = a.y + b.y;
    return a;
}

static __device__ __host__ inline CudaComplex operator/(CudaComplex a, CudaComplex b)
{
    CudaComplex c;
    const float d = b.x * b.x + b.y * b.y;
    c.x = (a.x * b.x + a.y * b.y) / d;
    c.y = (a.y * b.x - a.x * b.y) / d;
    return c;
}

static __device__ __host__ inline bool operator==(CudaComplex a, CudaComplex b)
{
    return a.x == b.x && a.y == b.y;
}

static __device__ __host__ inline bool operator!=(CudaComplex a, CudaComplex b)
{
    //The original required BOTH components to differ, so any candidate
    //sharing one coordinate with the excluded point was wrongly treated
    //as equal (frequent in grid-generated clouds).
    return !(a == b);
}

} // namespace qftbx

#endif // QFTBX_CUDA_UTILITY_CUH
