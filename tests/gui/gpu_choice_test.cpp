/**
 * @file
 * @brief The GPU in a form: hidden without GPU support, disabled with the
 * reason when the card cannot be used, and the GPU only when it is chosen
 * on a card that can. A choice asked for goes to the CPU when the GPU
 * cannot be had. In the window, the templates start with the settings'
 * choice and every step opens with the last one the user made.
 */

#include <gtest/gtest.h>

#include <memory>

#include <QAction>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QWidget>

#include "src/core/pipeline/gpu_provider.h"
#include "src/core/project/settings.h"
#include "src/gui/application/error_message.h"
#include "src/gui/application/main_window.h"
#include "src/gui/boundaries/boundary_grid_form.h"
#include "src/gui/common/gpu_choice.h"
#include "src/gui/templates/templates_form.h"

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

TEST(GpuChoice, AChoiceGoesToTheCpuWhenTheGpuCannotBeHad)
{
    QWidget pair;
    QRadioButton & gpu = *new QRadioButton(&pair);
    QRadioButton & cpu = *new QRadioButton(&pair);
    {
        const ProviderInstalled installed(GpuAvailability::Usable);
        offerGpu(gpu, &cpu);
        chooseProcessor(gpu, cpu, Processor::Gpu);
        EXPECT_EQ(processorChosen(gpu), Processor::Gpu);
        chooseProcessor(gpu, cpu, Processor::Cpu);
        EXPECT_TRUE(cpu.isChecked());
        EXPECT_EQ(processorChosen(gpu), Processor::Cpu);
    }
    {
        const ProviderInstalled installed(GpuAvailability::NoDevice);
        offerGpu(gpu, &cpu);
        chooseProcessor(gpu, cpu, Processor::Gpu);
        EXPECT_TRUE(cpu.isChecked());
        EXPECT_EQ(processorChosen(gpu), Processor::Cpu);
    }
}

TEST(GpuChoice, EveryStepOpensWithTheLastChoiceMade)
{
    const ProviderInstalled installed(GpuAvailability::Usable);
    QStringList reported;
    const ErrorReporter previous = setErrorReporter(
            [&reported](const QString & message, const QString & title) { reported.push_back(title + ": " + message); });

    Settings settings;
    settings.defaults.templatesOnGpu = true;
    {
        MainWindow window(settings);
        window.setFileChooser([](bool) { return QString(QFTBX_TEST_DATA_DIR "/planta1.qft"); });
        window.findChild<QAction *>("actionOpen")->trigger();

        const auto radio = [](QWidget * form, const char * name) {
            return form->findChild<QRadioButton *>(QString::fromUtf8(name));
        };

        window.findChild<QPushButton *>("templatesButton")->click();
        TemplatesForm * templates = window.findChild<TemplatesForm *>();
        ASSERT_NE(templates, nullptr);
        EXPECT_TRUE(radio(templates, "gpuRadio")->isChecked()) << "the templates start with the settings' choice";

        radio(templates, "cpuRadio")->click();
        window.findChild<QPushButton *>("boundariesButton")->click();
        BoundaryGridForm * boundaries = window.findChild<BoundaryGridForm *>();
        ASSERT_NE(boundaries, nullptr);
        EXPECT_TRUE(radio(boundaries, "cpuRadio")->isChecked()) << "the boundaries take the choice of the templates";

        radio(boundaries, "gpuRadio")->click();
        window.findChild<QPushButton *>("templatesButton")->click();
        EXPECT_TRUE(radio(templates, "gpuRadio")->isChecked()) << "the last choice made holds in every step";
    }
    {
        MainWindow window;
        window.setFileChooser([](bool) { return QString(QFTBX_TEST_DATA_DIR "/planta1.qft"); });
        window.findChild<QAction *>("actionOpen")->trigger();
        window.findChild<QPushButton *>("templatesButton")->click();
        TemplatesForm * templates = window.findChild<TemplatesForm *>();
        ASSERT_NE(templates, nullptr);
        EXPECT_TRUE(templates->findChild<QRadioButton *>("cpuRadio")->isChecked()) << "the compiled default is the CPU";
    }
    setErrorReporter(previous);
    EXPECT_TRUE(reported.isEmpty()) << reported.join("\n").toStdString();
}
