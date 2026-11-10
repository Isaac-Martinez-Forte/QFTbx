/**
 * @file
 * @brief The probe of the card.
 *
 * The runtime is linked statically, so its version is the one compiled
 * against. A driver older than the runtime and a missing driver both make
 * the device count fail; the driver version tells them apart. Of several
 * cards the one with most multiprocessors is taken, the first on a tie.
 * The default memory pool of the card keeps what it has reserved instead
 * of handing it back at every synchronisation, so the reservations that
 * follow each other in a computation cost nothing after the first.
 */

#include "src/core/gpu/common/device_probe.h"

#include <cstdint>
#include <mutex>

#include <cuda_runtime.h>

#include "src/core/gpu/common/check.cuh"

namespace qftbx::gpu {

namespace {

__global__ void answer(int * out)
{
    *out = 42;
}

int attribute(cudaDeviceAttr which, int device)
{
    int value = 0;
    check(cudaDeviceGetAttribute(&value, which, device), "cudaDeviceGetAttribute");
    return value;
}

DeviceProbe::Outcome launchOutcome(std::string & detail)
{
    int * value = nullptr;
    check(cudaMalloc(&value, sizeof(int)), "cudaMalloc");
    answer<<<1, 1>>>(value);
    const cudaError_t launched = cudaGetLastError();
    int host = 0;
    const cudaError_t copied = launched == cudaSuccess
            ? cudaMemcpy(&host, value, sizeof(int), cudaMemcpyDeviceToHost) : launched;
    cudaFree(value);

    if (launched == cudaErrorNoKernelImageForDevice || launched == cudaErrorInvalidDeviceFunction) {
        return DeviceProbe::Outcome::NoCodeForCard;
    }
    if (copied != cudaSuccess || host != 42) {
        detail = cudaGetErrorString(copied);
        return DeviceProbe::Outcome::Failed;
    }
    return DeviceProbe::Outcome::Usable;
}

DeviceProbe look()
{
    DeviceProbe found;
    found.runtimeVersion = CUDART_VERSION;
    cudaDriverGetVersion(&found.driverVersion);

    int count = 0;
    const cudaError_t counted = cudaGetDeviceCount(&count);
    if (counted == cudaErrorInsufficientDriver) {
        found.outcome = found.driverVersion == 0 ? DeviceProbe::Outcome::NoDriver : DeviceProbe::Outcome::OldDriver;
        cudaGetLastError();
        return found;
    }
    if (counted == cudaErrorNoDevice || (counted == cudaSuccess && count == 0)) {
        found.outcome = DeviceProbe::Outcome::NoDevice;
        cudaGetLastError();
        return found;
    }
    if (counted != cudaSuccess) {
        found.outcome = DeviceProbe::Outcome::Failed;
        found.detail = cudaGetErrorString(counted);
        cudaGetLastError();
        return found;
    }

    try {
        int best = 0;
        for (int d = 1; d < count; ++d) {
            if (attribute(cudaDevAttrMultiProcessorCount, d) > attribute(cudaDevAttrMultiProcessorCount, best)) {
                best = d;
            }
        }

        cudaDeviceProp properties{};
        check(cudaGetDeviceProperties(&properties, best), "cudaGetDeviceProperties");
        found.device = best;
        found.name = properties.name;
        found.computeMajor = properties.major;
        found.computeMinor = properties.minor;
        found.memory = properties.totalGlobalMem;
        found.multiprocessors = properties.multiProcessorCount;
        found.doublePrecisionRatio = attribute(cudaDevAttrSingleToDoublePrecisionPerfRatio, best);
        found.memoryPools = attribute(cudaDevAttrMemoryPoolsSupported, best) != 0;

        check(cudaSetDevice(best), "cudaSetDevice");
        found.outcome = launchOutcome(found.detail);

        if (found.outcome == DeviceProbe::Outcome::Usable && found.memoryPools) {
            cudaMemPool_t pool = nullptr;
            check(cudaDeviceGetDefaultMemPool(&pool, best), "cudaDeviceGetDefaultMemPool");
            std::uint64_t keepAll = UINT64_MAX;
            check(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &keepAll),
                  "cudaMemPoolSetAttribute");
        }
    } catch (const CudaError & error) {
        found.outcome = DeviceProbe::Outcome::Failed;
        found.detail = error.what();
    }

    cudaGetLastError();
    return found;
}

}

const DeviceProbe & probe()
{
    static std::once_flag once;
    static DeviceProbe found;
    std::call_once(once, [] { found = look(); });
    return found;
}

void useProbedDevice()
{
    const DeviceProbe & found = probe();
    if (found.outcome != DeviceProbe::Outcome::Usable) {
        throw CudaError("the card cannot be used", 0);
    }
    check(cudaSetDevice(found.device), "cudaSetDevice");
}

}
