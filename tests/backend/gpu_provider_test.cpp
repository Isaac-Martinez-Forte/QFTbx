/**
 * @file
 * @brief The choice of the GPU through the provider, without a card.
 *
 * A stand-in provider plays every case. With none installed, asking for
 * the GPU is an input the build cannot take; with a card that cannot be
 * used, the computation fails with the reason; with a usable card whose
 * provider lacks the engine of a phase, it fails saying so. None of them
 * publishes anything or loses what the project held. A provider that has
 * the engine is the one the stage computes with. Boundaries on the GPU from
 * the whole clouds are given the clouds on the card: those the template
 * engine keeps, or else uploaded once until the templates change; from the
 * contours or on the processor, none. The reasons name the versions as
 * CUDA writes them, 12040 being 12.4.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/boundaries/boundary_engine.h"
#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/pipeline/gpu_provider.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/templates/template_engine.h"

using namespace qftbx;

namespace {

class StandInClouds : public ResidentClouds
{
public:
    std::size_t frequencies() const override { return 3; }
    std::size_t plants() const override { return 9; }
};

class KeepingTemplateEngine : public TemplateEngine
{
public:
    explicit KeepingTemplateEngine(std::shared_ptr<const ResidentClouds> kept) : m_kept(std::move(kept)) {}

    std::shared_ptr<const ResidentClouds> residentClouds() const override { return m_kept; }

private:
    std::shared_ptr<const ResidentClouds> m_kept;
};

class TellingBoundaryEngine : public BoundaryEngine
{
public:
    explicit TellingBoundaryEngine(std::vector<std::shared_ptr<const ResidentClouds>> & given) : m_given(given) {}

    void setResidentClouds(std::shared_ptr<const ResidentClouds> clouds) override { m_given.push_back(clouds); }

private:
    std::vector<std::shared_ptr<const ResidentClouds>> & m_given;
};

class StandInProvider : public GpuProvider
{
public:
    explicit StandInProvider(GpuAvailability availability, bool engines = false)
        : m_engines(engines)
    {
        m_status.availability = availability;
        m_status.name = "Stand-in";
        m_status.computeMajor = 8;
        m_status.computeMinor = 6;
        m_status.driverVersion = 12040;
        m_status.runtimeVersion = 13000;
        m_status.detail = "out of order";
    }

    const GpuStatus & status() const override { return m_status; }

    std::unique_ptr<TemplateComputation> templateComputation() const override
    {
        if (!m_engines) {
            return nullptr;
        }
        ++templateEngines;
        if (kept != nullptr) {
            return std::make_unique<KeepingTemplateEngine>(kept);
        }
        return std::make_unique<TemplateEngine>();
    }

    std::unique_ptr<BoundaryComputation> boundaryComputation() const override
    {
        if (!m_engines) {
            return nullptr;
        }
        ++boundaryEngines;
        return std::make_unique<TellingBoundaryEngine>(given);
    }

    std::shared_ptr<const ResidentClouds> cloudsOnCard(const CloudSet & clouds) const override
    {
        ++uploads;
        uploadedFrequencies = clouds.size();
        return std::make_shared<StandInClouds>();
    }

    mutable int templateEngines = 0;
    mutable int boundaryEngines = 0;
    mutable int uploads = 0;
    mutable std::size_t uploadedFrequencies = 0;
    mutable std::vector<std::shared_ptr<const ResidentClouds>> given;
    std::shared_ptr<const ResidentClouds> kept;

private:
    GpuStatus m_status;
    bool m_engines = false;
};

class ProviderInstalled
{
public:
    explicit ProviderInstalled(std::unique_ptr<GpuProvider> provider)
    {
        installGpuProvider(std::move(provider));
    }

    ~ProviderInstalled()
    {
        installGpuProvider(nullptr);
    }
};

void publishProblem(ProjectController & controller)
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(std::string("a"), Range(1.0, 2.0), 1.5), Parameter(1.0)};
    controller.setPlant(std::make_unique<PolynomialForm>(std::string("P"), numerator, denominator,
                                                         Parameter(std::string("kv"), Range(1.0, 2.0), 1.5),
                                                         Parameter(0.0)));

    SpecificationRecords records;
    SpecificationRecord & stability = records.at(static_cast<std::size_t>(SpecificationType::Stability));
    stability.name = specificationName(SpecificationType::Stability);
    stability.used = true;
    stability.constant = true;
    stability.height = 5.0;
    stability.omegaStart = 0.1;
    stability.omegaEnd = 10.0;
    controller.setSpecifications(std::move(records));

    controller.setOmega(std::make_unique<Omega>(0.1, 10.0, 3, logspace(-1.0, 1.0, 3), Omega::LogSpace));
}

ParameterGrids grids()
{
    ParameterGrids sweep;
    sweep[std::string("a")] = math::linspace(1.0, 2.0, 3);
    sweep[std::string("kv")] = math::linspace(1.0, 2.0, 3);
    return sweep;
}

bool computeBoundaries(ProjectController & controller, Processor processor, bool fromContour = true)
{
    return controller.computeBoundaries(Range(-360.0, 0.0), 37, Range(-40.0, 40.0), 21, -1.0, fromContour,
                                        processor);
}

}

TEST(GpuProvider, WithoutGpuSupportTheGpuIsRefusedAsInput)
{
    ASSERT_EQ(gpuProvider(), nullptr);
    EXPECT_THROW(requireGpu(), InvalidInput);

    ProjectController controller;
    publishProblem(controller);
    EXPECT_THROW(controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Gpu), InvalidInput);
    EXPECT_TRUE(controller.templates().empty());
}

TEST(GpuProvider, ACardThatCannotBeUsedFailsWithItsReason)
{
    const ProviderInstalled installed(std::make_unique<StandInProvider>(GpuAvailability::NoDevice));

    ProjectController controller;
    publishProblem(controller);
    try {
        controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Gpu);
        FAIL() << "a card that cannot be used must not compute";
    } catch (const ComputationError & error) {
        EXPECT_EQ(std::string(error.what()), "No NVIDIA card was found.");
    }
    EXPECT_TRUE(controller.templates().empty());
}

TEST(GpuProvider, APhaseWithoutAGpuEngineFailsAndKeepsWhatTheProjectHeld)
{
    const ProviderInstalled installed(std::make_unique<StandInProvider>(GpuAvailability::Usable));

    ProjectController controller;
    publishProblem(controller);
    EXPECT_THROW(controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Gpu),
                 ComputationError);
    EXPECT_TRUE(controller.templates().empty());

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids()));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Cpu));
    ASSERT_NE(controller.boundaries(), nullptr);

    EXPECT_THROW(computeBoundaries(controller, Processor::Gpu), ComputationError);
    EXPECT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty());

    EXPECT_TRUE(computeBoundaries(controller, Processor::Cpu));
}

TEST(GpuProvider, TheStagesComputeWithTheEnginesOfTheProvider)
{
    auto provider = std::make_unique<StandInProvider>(GpuAvailability::Usable, true);
    const StandInProvider & counts = *provider;
    const ProviderInstalled installed(std::move(provider));

    ProjectController controller;
    publishProblem(controller);
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Gpu));
    EXPECT_EQ(counts.templateEngines, 1);
    EXPECT_EQ(controller.templates().size(), 3u);

    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu));
    EXPECT_EQ(counts.boundaryEngines, 1);
    ASSERT_TRUE(computeBoundaries(controller, Processor::Cpu));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu));
    EXPECT_EQ(counts.boundaryEngines, 2);
}

TEST(GpuProvider, TheBoundariesOfTheGpuReadTheCloudsTheCardHolds)
{
    auto provider = std::make_unique<StandInProvider>(GpuAvailability::Usable, true);
    StandInProvider & stand = *provider;
    const ProviderInstalled installed(std::move(provider));

    ProjectController controller;
    publishProblem(controller);
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Gpu));

    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu, true));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu, false));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu, false));
    ASSERT_EQ(stand.given.size(), 3u);
    EXPECT_EQ(stand.given[0], nullptr);
    ASSERT_NE(stand.given[1], nullptr);
    EXPECT_EQ(stand.given[2], stand.given[1]);
    EXPECT_EQ(stand.uploads, 1);
    EXPECT_EQ(stand.uploadedFrequencies, 3u);

    ASSERT_TRUE(computeBoundaries(controller, Processor::Cpu, false));
    EXPECT_EQ(stand.uploads, 1);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Cpu));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu, false));
    EXPECT_EQ(stand.uploads, 2);
    EXPECT_NE(stand.given.back(), stand.given[1]);

    stand.kept = std::make_shared<StandInClouds>();
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids(), Processor::Gpu));
    ASSERT_TRUE(computeBoundaries(controller, Processor::Gpu, false));
    EXPECT_EQ(stand.given.back(), stand.kept);
    EXPECT_EQ(stand.uploads, 2);
}

TEST(GpuProvider, EachReasonSaysWhatIsMissing)
{
    const auto reasonOf = [](GpuAvailability availability) {
        return StandInProvider(availability).status().reason().rendered();
    };

    EXPECT_EQ(reasonOf(GpuAvailability::Usable), "");
    EXPECT_EQ(reasonOf(GpuAvailability::NoDriver), "No NVIDIA driver was found.");
    EXPECT_EQ(reasonOf(GpuAvailability::NoDevice), "No NVIDIA card was found.");
    EXPECT_EQ(reasonOf(GpuAvailability::OldDriver),
              "The NVIDIA driver supports CUDA 12.4, and this build needs CUDA 13.0: update the driver.");
    EXPECT_EQ(reasonOf(GpuAvailability::NoCodeForCard),
              "This build has no code for the Stand-in (compute capability 8.6): build it for that card.");
    EXPECT_EQ(reasonOf(GpuAvailability::Failed), "The GPU cannot be used: out of order");
}
