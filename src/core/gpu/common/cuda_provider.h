/**
 * @file
 * @brief The provider of the CUDA engines.
 *
 * Installed by the application at start in a build with CUDA. It reads the
 * card the first time it is asked, not at start, so a program that never
 * offers the GPU never initialises CUDA. The engines it gives are those
 * built so far: the templates and the boundaries; a failure putting clouds
 * on the card is a ComputationError with CUDA's text.
 */

#ifndef QFTBX_GPU_CUDA_PROVIDER_H
#define QFTBX_GPU_CUDA_PROVIDER_H

#include <memory>
#include <mutex>

#include "src/core/pipeline/gpu_provider.h"

namespace qftbx::gpu {

class CudaProvider : public GpuProvider
{
public:
    const GpuStatus & status() const override;

    std::unique_ptr<TemplateComputation> templateComputation() const override;
    std::unique_ptr<BoundaryComputation> boundaryComputation() const override;
    std::shared_ptr<const ResidentClouds> cloudsOnCard(const CloudSet & clouds) const override;

private:
    mutable std::once_flag m_once;
    mutable GpuStatus m_status;
};

std::unique_ptr<GpuProvider> makeCudaProvider();

}

#endif
