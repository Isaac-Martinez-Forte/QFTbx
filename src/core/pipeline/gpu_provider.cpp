/**
 * @file
 * @brief The one provider of GPU engines the program may have installed,
 * and the sentences that say why a card cannot be used.
 *
 * CUDA writes its versions as a thousand times the major plus ten times the
 * minor, 12040 for 12.4.
 */

#include "src/core/pipeline/gpu_provider.h"

#include "src/core/common/exception.h"
#include "src/core/boundaries/boundary_computation.h"
#include "src/core/templates/template_computation.h"

namespace qftbx {

namespace {

std::unique_ptr<GpuProvider> & installed()
{
    static std::unique_ptr<GpuProvider> provider;
    return provider;
}

std::string cudaVersion(int version)
{
    return std::to_string(version / 1000) + "." + std::to_string((version % 1000) / 10);
}

}

Message GpuStatus::reason() const
{
    switch (availability) {
    case GpuAvailability::Usable:
        return Message();
    case GpuAvailability::NoDriver:
        return QFTBX_TR("Core", "No NVIDIA driver was found.");
    case GpuAvailability::NoDevice:
        return QFTBX_TR("Core", "No NVIDIA card was found.");
    case GpuAvailability::OldDriver:
        return QFTBX_TR("Core", "The NVIDIA driver supports CUDA %1, and this build needs CUDA %2: update the driver.")
                .arg(cudaVersion(driverVersion)).arg(cudaVersion(runtimeVersion));
    case GpuAvailability::NoCodeForCard:
        return QFTBX_TR("Core", "This build has no code for the %1 (compute capability %2.%3): build it for that card.")
                .arg(name).arg(computeMajor).arg(computeMinor);
    case GpuAvailability::Failed:
        break;
    }
    return QFTBX_TR("Core", "The GPU cannot be used: %1").arg(detail);
}

std::unique_ptr<TemplateComputation> GpuProvider::templateComputation() const
{
    return nullptr;
}

std::unique_ptr<BoundaryComputation> GpuProvider::boundaryComputation() const
{
    return nullptr;
}

void installGpuProvider(std::unique_ptr<GpuProvider> provider)
{
    installed() = std::move(provider);
}

const GpuProvider * gpuProvider()
{
    return installed().get();
}

const GpuProvider & requireGpu()
{
    const GpuProvider * provider = gpuProvider();
    if (provider == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "This build of QFTbx has no GPU support: compute on the processor."));
    }
    if (!provider->status().usable()) {
        throw ComputationError(provider->status().reason());
    }
    return *provider;
}

Message gpuFailure(const std::string & detail)
{
    return QFTBX_TR("Core", "The GPU failed: %1").arg(detail);
}

}
