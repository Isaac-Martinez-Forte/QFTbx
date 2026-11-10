/**
 * @file
 * @brief The GPU in a form: hidden without GPU support, disabled with the
 * reason when the card cannot be used, and the GPU only when it is chosen
 * on a card that can.
 */

#include <gtest/gtest.h>

#include <memory>

#include <QCheckBox>
#include <QRadioButton>

#include "src/core/pipeline/gpu_provider.h"
#include "src/gui/application/error_message.h"
#include "src/gui/common/gpu_choice.h"

using namespace qftbx;

namespace {

class StandInProvider : public GpuProvider
{
public:
    explicit StandInProvider(GpuAvailability availability)
    {
        m_status.availability = availability;
    }

    const GpuStatus & status() const override { return m_status; }

private:
    GpuStatus m_status;
};

class ProviderInstalled
{
public:
    explicit ProviderInstalled(GpuAvailability availability)
    {
        installGpuProvider(std::make_unique<StandInProvider>(availability));
    }

    ~ProviderInstalled()
    {
        installGpuProvider(nullptr);
    }
};

}

TEST(GpuChoice, WithoutGpuSupportTheChoiceIsHidden)
{
    ASSERT_EQ(gpuProvider(), nullptr);

    QRadioButton gpu;
    QRadioButton cpu;
    offerGpu(gpu, &cpu);
    EXPECT_TRUE(gpu.isHidden());
    EXPECT_TRUE(cpu.isHidden());

    gpu.setChecked(true);
    EXPECT_EQ(processorChosen(gpu), Processor::Cpu);
}

TEST(GpuChoice, ACardThatCannotBeUsedIsOfferedDisabledWithTheReason)
{
    const ProviderInstalled installed(GpuAvailability::NoDevice);

    QCheckBox gpu;
    gpu.setChecked(true);
    offerGpu(gpu);
    EXPECT_FALSE(gpu.isHidden());
    EXPECT_FALSE(gpu.isEnabled());
    EXPECT_FALSE(gpu.isChecked());
    EXPECT_EQ(gpu.toolTip(), translated(gpuProvider()->status().reason()));

    gpu.setChecked(true);
    EXPECT_EQ(processorChosen(gpu), Processor::Cpu);
}

TEST(GpuChoice, OnAUsableCardTheGpuIsWhatIsChosen)
{
    const ProviderInstalled installed(GpuAvailability::Usable);

    QCheckBox gpu;
    gpu.setToolTip("Sweep the family on the GPU.");
    offerGpu(gpu);
    EXPECT_TRUE(gpu.isEnabled());
    EXPECT_EQ(gpu.toolTip(), "Sweep the family on the GPU.");

    EXPECT_EQ(processorChosen(gpu), Processor::Cpu);
    gpu.setChecked(true);
    EXPECT_EQ(processorChosen(gpu), Processor::Gpu);
}
