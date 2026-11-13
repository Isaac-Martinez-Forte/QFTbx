/**
 * @file
 * @brief The provider of the CUDA engines, from the probe of the card.
 */

#include "src/core/gpu/common/cuda_provider.h"

#include "src/core/common/exception.h"
#include "src/core/gpu/common/cuda_error.h"
#include "src/core/gpu/common/device_probe.h"
#include "src/core/gpu/boundaries/gpu_boundary_engine.h"
#include "src/core/gpu/templates/gpu_template_engine.h"
#include "src/core/gpu/templates/walk_preparation.h"

namespace qftbx::gpu {

namespace {

GpuAvailability availabilityOf(DeviceProbe::Outcome outcome)
{
    switch (outcome) {
    case DeviceProbe::Outcome::Usable: return GpuAvailability::Usable;
    case DeviceProbe::Outcome::NoDriver: return GpuAvailability::NoDriver;
    case DeviceProbe::Outcome::NoDevice: return GpuAvailability::NoDevice;
    case DeviceProbe::Outcome::OldDriver: return GpuAvailability::OldDriver;
    case DeviceProbe::Outcome::NoCodeForCard: return GpuAvailability::NoCodeForCard;
    case DeviceProbe::Outcome::Failed: break;
    }
    return GpuAvailability::Failed;
}

}

const GpuStatus & CudaProvider::status() const
{
    std::call_once(m_once, [this] {
        const DeviceProbe & found = probe();
        m_status.availability = availabilityOf(found.outcome);
        m_status.name = found.name;
        m_status.computeMajor = found.computeMajor;
        m_status.computeMinor = found.computeMinor;
        m_status.memory = found.memory;
        m_status.multiprocessors = found.multiprocessors;
        m_status.doublePrecisionRatio = found.doublePrecisionRatio;
        m_status.driverVersion = found.driverVersion;
        m_status.runtimeVersion = found.runtimeVersion;
        m_status.detail = found.detail;
    });
    return m_status;
}

std::unique_ptr<TemplateComputation> CudaProvider::templateComputation() const
{
    return std::make_unique<GpuTemplateEngine>();
}

std::unique_ptr<BoundaryComputation> CudaProvider::boundaryComputation() const
{
    return std::make_unique<GpuBoundaryEngine>();
}

std::shared_ptr<const ResidentClouds> CudaProvider::cloudsOnCard(const CloudSet & clouds) const
{
    try {
        return residentOf(gpu::cloudsOnCard(clouds));
    } catch (const CudaError & failure) {
        throw ComputationError(gpuFailure(failure.what()));
    }
}

std::unique_ptr<GpuProvider> makeCudaProvider()
{
    return std::make_unique<CudaProvider>();
}

}
