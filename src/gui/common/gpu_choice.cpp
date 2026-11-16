/**
 * @file
 * @brief The choice of the GPU in a form, from what the provider found.
 */

#include "src/gui/common/gpu_choice.h"

#include "src/core/pipeline/gpu_provider.h"
#include "src/gui/application/error_message.h"

namespace qftbx {

void offerGpu(QAbstractButton & gpu, QWidget * companion)
{
    const GpuProvider * provider = gpuProvider();

    gpu.setVisible(provider != nullptr);
    if (companion != nullptr) {
        companion->setVisible(provider != nullptr);
    }
    if (provider == nullptr) {
        return;
    }

    const GpuStatus & status = provider->status();
    gpu.setEnabled(status.usable());
    if (!status.usable()) {
        gpu.setChecked(false);
        gpu.setToolTip(translated(status.reason()));
    }
}

void chooseProcessor(QAbstractButton & gpu, QAbstractButton & cpu, Processor processor)
{
    const bool onGpu = processor == Processor::Gpu && !gpu.isHidden() && gpu.isEnabled();
    (onGpu ? gpu : cpu).setChecked(true);
}

Processor processorChosen(const QAbstractButton & gpu)
{
    return !gpu.isHidden() && gpu.isEnabled() && gpu.isChecked() ? Processor::Gpu : Processor::Cpu;
}

}
