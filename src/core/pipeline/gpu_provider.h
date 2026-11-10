/**
 * @file
 * @brief The GPU engines, when the program has them, behind one provider.
 *
 * The core knows the GPU only through this interface. A build with CUDA
 * installs a provider when the program starts; one without leaves none.
 * status() is what the provider found on the card, read once; reason()
 * turns it into a sentence for the user, here so that the translation is
 * the same whatever the build. A provider answers null for a phase it has
 * no engine for. requireGpu() is what a stage calls before asking for an
 * engine: it throws InvalidInput when the build has no GPU support and
 * ComputationError, with the reason, when the card cannot be used.
 * gpuFailure() is the message of a failure on the card during a
 * computation, with CUDA's own text.
 */

#ifndef QFTBX_GPU_PROVIDER_H
#define QFTBX_GPU_PROVIDER_H

#include <cstddef>
#include <memory>
#include <string>

#include "src/core/common/message.h"

namespace qftbx {

class TemplateComputation;
class BoundaryComputation;

enum class GpuAvailability { Usable, NoDriver, NoDevice, OldDriver, NoCodeForCard, Failed };

struct GpuStatus
{
    GpuAvailability availability = GpuAvailability::NoDevice;
    std::string name;
    int computeMajor = 0;
    int computeMinor = 0;
    std::size_t memory = 0;
    int multiprocessors = 0;
    int doublePrecisionRatio = 0;
    int driverVersion = 0;
    int runtimeVersion = 0;
    std::string detail;

    bool usable() const { return availability == GpuAvailability::Usable; }
    Message reason() const;
};

class GpuProvider
{
public:
    virtual ~GpuProvider() = default;

    virtual const GpuStatus & status() const = 0;

    virtual std::unique_ptr<TemplateComputation> templateComputation() const;
    virtual std::unique_ptr<BoundaryComputation> boundaryComputation() const;
};

void installGpuProvider(std::unique_ptr<GpuProvider> provider);

const GpuProvider * gpuProvider();

const GpuProvider & requireGpu();

Message gpuFailure(const std::string & detail);

}

#endif
