/**
 * @file
 * @brief The kernels of the GPU tests.
 *
 * The spinning kernel waits on the cancel flag for at most a few seconds
 * of the card's clock, so a broken flag fails the test instead of hanging
 * it; it writes 1 when the flag stopped it and 2 when the clock did.
 */

#include "tests/gpu/test_kernels.h"

#include <chrono>
#include <thread>

#include "src/core/gpu/common/cancel_flag.cuh"
#include "src/core/gpu/common/device_buffer.cuh"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/common/launch.cuh"
#include "src/core/gpu/common/stream.cuh"

namespace qftbx::gputest {

namespace {

__global__ void doubled(double * values, std::size_t count)
{
    const std::size_t i = blockIdx.x * static_cast<std::size_t>(blockDim.x) + threadIdx.x;
    if (i < count) {
        values[i] *= 2.0;
    }
}

__global__ void spin(int * flag, int * stopped, long long cycles)
{
    const long long start = clock64();
    while (clock64() - start < cycles) {
        if (gpu::cancelRequested(flag)) {
            *stopped = 1;
            return;
        }
    }
    *stopped = 2;
}

}

std::vector<double> doubledOnTheCard(const std::vector<double> & values)
{
    if (values.empty()) {
        return {};
    }
    gpu::useProbedDevice();
    gpu::Stream stream;
    gpu::DeviceBuffer<double> device(values.size(), stream);
    device.upload(values.data());

    const int block = gpu::blockSizeFor(doubled);
    doubled<<<gpu::blocksFor(values.size(), block), block, 0, stream.get()>>>(device.data(), values.size());
    gpu::checkLaunch("doubled");

    std::vector<double> result(values.size());
    device.download(result.data());
    stream.synchronize();
    return result;
}

bool reservationsFollowEachOther(std::size_t count, std::size_t elements)
{
    gpu::useProbedDevice();
    gpu::Stream stream;
    for (std::size_t i = 0; i < count; ++i) {
        gpu::DeviceBuffer<double> buffer(elements, stream);
        buffer.fill(0);
    }
    stream.synchronize();
    return true;
}

SpinOutcome spinUntilCancelled(CancellationToken & token)
{
    gpu::useProbedDevice();
    gpu::Stream stream;
    gpu::CancelFlag flag;
    gpu::DeviceBuffer<int> stopped(1, stream);
    stopped.fill(0);

    int clockKhz = 0;
    gpu::check(cudaDeviceGetAttribute(&clockKhz, cudaDevAttrClockRate, gpu::probe().device), "clock rate");
    spin<<<1, 1, 0, stream.get()>>>(flag.device(), stopped.data(), 5000LL * clockKhz);
    gpu::checkLaunch("spin");

    std::thread canceller([&token] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        token.cancel();
    });

    SpinOutcome outcome;
    outcome.finished = stream.finish(&token, &flag);
    canceller.join();

    int host = 0;
    stopped.download(&host);
    stream.synchronize();
    outcome.stoppedOnTheFlag = host == 1;
    return outcome;
}

}
