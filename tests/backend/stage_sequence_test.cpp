/**
 * @file
 * @brief The seven stages of the pipeline walked from nothing, in order.
 *
 * A plant, the specifications, the frequencies, the templates, the boundaries,
 * a controller structure and the search are published or computed in order on
 * a deliberately tiny problem; each stage must produce its artefact and leave
 * the one before it standing. A null publish and a reserved parameter name are
 * refused at publication, and recomputing a stage drops exactly what depends
 * on it. Cancellation travels out as its own exception, publishes nothing and
 * does not linger into the next run; a search off the calling thread closes
 * the project while it runs and reports its failures instead of throwing from
 * the worker. The completed steps are derived from what the project holds, a
 * loaded file replaces the project, and every change announces exactly once.
 */

#include "src/core/loopshaping/loop_shaping_types.h"
#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/pipeline/cancellation.h"
#include "src/core/pipeline/pipeline_step.h"
#include "src/app/project_controller.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"

using namespace qftbx;

namespace {

std::unique_ptr<LtiSystem> makePlant()
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    return std::make_unique<PolynomialForm>(std::string("P"), numerator, denominator,
                              Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
                              Parameter(0.0));
}

std::unique_ptr<LtiSystem> makeControllerStructure()
{
    std::vector<Parameter> numerator{
        Parameter(std::string("z"), qftbx::Range(0.1, 10.0), 1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("p"), qftbx::Range(0.1, 10.0), 1.0)};

    return std::make_unique<ZeroPoleGain>(std::string("K"), numerator, denominator,
                              Parameter(std::string("kc"), qftbx::Range(0.01, 100.0), 1.0),
                              Parameter(0.0));
}

std::unique_ptr<Omega> makeOmega()
{
    return std::make_unique<Omega>(0.1, 10.0, 3, qftbx::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

qftbx::ParameterGrids makeGrids()
{
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
    return grids;
}

qftbx::SpecificationRecords makeSpecifications()
{
    qftbx::SpecificationRecords records;

    qftbx::SpecificationRecord & stability =
            records.at(static_cast<std::size_t>(qftbx::SpecificationType::Stability));
    stability.name = qftbx::specificationName(qftbx::SpecificationType::Stability);
    stability.used = true;
    stability.constant = true;
    stability.system = nullptr;
    stability.height = 5.0;
    stability.omegaStart = 0.1;
    stability.omegaEnd = 10.0;

    return records;
}

}

TEST(StageSequence, TheSevenStagesWalkedFromNothing)
{
    ProjectController controller;

    EXPECT_TRUE(controller.setPlant(makePlant()));
    ASSERT_NE(controller.plant(), nullptr);

    controller.setSpecifications(makeSpecifications());
    ASSERT_NE(controller.specifications(), nullptr);

    EXPECT_TRUE(controller.setOmega(makeOmega()));
    ASSERT_NE(controller.omega(), nullptr);
    ASSERT_EQ(controller.omega()->values()->size(), 3u);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_EQ(controller.templates().size(), 3u);
    for (const qftbx::ComplexCloud & cloud : controller.templates()) {
        EXPECT_FALSE(cloud.empty());
    }
    EXPECT_NE(controller.plant(), nullptr);
    EXPECT_NE(controller.omega(), nullptr);

    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    ASSERT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty())
        << "computing the boundaries must not disturb the templates they came from";

    EXPECT_TRUE(controller.setControllerStructure(makeControllerStructure()));
    ASSERT_NE(controller.controllerStructure(), nullptr);
    EXPECT_NE(controller.boundaries(), nullptr);

    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt,
                                              qftbx::Range(1e-3, 100.0), 100));
    ASSERT_NE(controller.loopShapingResult(), nullptr);

    LtiSystem * const designed = controller.loopShapingResult()->controller();
    ASSERT_NE(designed, nullptr);

    EXPECT_EQ(designed->gain().range().min, designed->gain().range().max);
    EXPECT_GT(designed->gain().range().min, 0.0);

    EXPECT_NE(controller.plant(), nullptr);
    EXPECT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty());
}

TEST(StageSequence, ANullStepIsRefusedInsteadOfWipingTheProject)
{
    ProjectController controller;

    EXPECT_THROW(controller.setPlant(nullptr), qftbx::InvalidInput);
    EXPECT_THROW(controller.setOmega(nullptr), qftbx::InvalidInput);
    EXPECT_THROW(controller.setControllerStructure(nullptr), qftbx::InvalidInput);
    EXPECT_THROW(controller.setSpecifications(std::nullopt), qftbx::InvalidInput);
}

TEST(StageSequence, ReservedParameterNamesAreRefusedWhenPublished)
{
    for (const char * reserved : {"sin", "sqrt", "log", "pi", "PI", "e", "E", "s"}) {
        ProjectController controller;

        std::vector<Parameter> numerator{Parameter(1.0)};
        std::vector<Parameter> denominator{Parameter(1.0)};
        auto plant = std::make_unique<PolynomialForm>(
                    std::string("P"), numerator, denominator,
                    Parameter(std::string(reserved), qftbx::Range(1.0, 2.0), 1.5),
                    Parameter(0.0));

        EXPECT_THROW(controller.setPlant(std::move(plant)), qftbx::InvalidInput)
            << "a parameter named \"" << reserved << "\" has to be refused";
    }

    ProjectController controller;
    auto plant = std::make_unique<PolynomialForm>(
                std::string("P"), std::vector<Parameter>{Parameter(1.0)},
                std::vector<Parameter>{Parameter(1.0)},
                Parameter(std::string("k"), qftbx::Range(1.0, 2.0), 1.5),
                Parameter(0.0));
    EXPECT_NO_THROW(controller.setPlant(std::move(plant)));
}

TEST(StageSequence, AConstantKeepsItsNumericName)
{
    ProjectController controller;

    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{Parameter(2.5), Parameter(1.0)};
    auto plant = std::make_unique<PolynomialForm>(
                std::string("P"), numerator, denominator,
                Parameter(1.0), Parameter(0.0));

    EXPECT_NO_THROW(controller.setPlant(std::move(plant)));
    EXPECT_NE(controller.plant(), nullptr);
}

TEST(StageSequence, RecomputingTheTemplatesDropsTheBoundaries)
{
    ProjectController controller;

    controller.setPlant(makePlant());
    controller.setSpecifications(makeSpecifications());
    controller.setOmega(makeOmega());

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    ASSERT_NE(controller.boundaries(), nullptr);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 8.0), makeGrids(), false));

    EXPECT_EQ(controller.boundaries(), nullptr)
        << "boundaries computed from the previous templates must not survive a new sweep";
    EXPECT_FALSE(controller.templates().empty());
}

TEST(StageSequence, RecomputingTheBoundariesDropsTheLoopShaping)
{
    ProjectController controller;

    controller.setPlant(makePlant());
    controller.setSpecifications(makeSpecifications());
    controller.setOmega(makeOmega());
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    controller.setControllerStructure(makeControllerStructure());
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt,
                                              qftbx::Range(1e-3, 100.0), 100));
    ASSERT_NE(controller.loopShapingResult(), nullptr);

    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 25,
                                             qftbx::Range(-40.0, 40.0), 15,
                                             -1.0, false, false));

    EXPECT_EQ(controller.loopShapingResult(), nullptr)
        << "a design found against the previous boundaries must not survive them";
    EXPECT_NE(controller.boundaries(), nullptr);
}

namespace {

void prepareForSearch(ProjectController & controller)
{
    controller.setPlant(makePlant());
    controller.setSpecifications(makeSpecifications());
    controller.setOmega(makeOmega());
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    controller.setControllerStructure(makeControllerStructure());
}

}

TEST(Cancellation, ACancelledSearchGivesUpAndPublishesNothing)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    qftbx::CancellationToken token;
    token.cancel();

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::nt,
                                               qftbx::Range(1e-3, 100.0), 100,
                                               0, &token),
                 qftbx::Cancelled);

    EXPECT_EQ(controller.loopShapingResult(), nullptr)
        << "a search that gave up must not leave half a design behind";
}

TEST(Cancellation, ATokenDoesNotLingerIntoTheNextRun)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    qftbx::CancellationToken token;
    token.cancel();

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::nt,
                                               qftbx::Range(1e-3, 100.0), 100,
                                               0, &token),
                 qftbx::Cancelled);

    EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt,
                                              qftbx::Range(1e-3, 100.0), 100));
    EXPECT_NE(controller.loopShapingResult(), nullptr);
}

TEST(Cancellation, AResetTokenLetsTheSearchRun)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    qftbx::CancellationToken token;
    token.cancel();
    EXPECT_TRUE(token.cancelled());

    token.reset();
    EXPECT_FALSE(token.cancelled());

    EXPECT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt,
                                              qftbx::Range(1e-3, 100.0), 100,
                                              0, &token));
    EXPECT_NE(controller.loopShapingResult(), nullptr);
}

TEST(BackgroundSearch, ASearchRunsOffTheCallingThreadAndPublishesItsResult)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    std::atomic<bool> told{false};

    ASSERT_TRUE(controller.startLoopShaping(0.5, qftbx::nt,
                                            qftbx::Range(1e-3, 100.0), 100, 0,
                                            [&told]() { told.store(true); }));

    controller.waitForComputation();

    EXPECT_TRUE(told.load()) << "the finished handler has to be called";
    EXPECT_TRUE(controller.lastComputationProduced());
    EXPECT_FALSE(controller.lastComputationCancelled());
    EXPECT_TRUE(controller.lastComputationError().empty());
    EXPECT_NE(controller.loopShapingResult(), nullptr);
    EXPECT_FALSE(controller.isComputing());
}

TEST(BackgroundSearch, TheProjectRefusesToChangeWhileASearchRuns)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    ASSERT_TRUE(controller.startLoopShaping(0.5, qftbx::nt,
                                            qftbx::Range(1e-3, 100.0), 100));

    if (controller.isComputing()) {
        EXPECT_THROW(controller.setPlant(makePlant()), qftbx::InvalidInput);
        EXPECT_THROW(controller.load(std::string("/nonexistent.qft")),
                     qftbx::InvalidInput);
        EXPECT_FALSE(controller.startLoopShaping(0.5, qftbx::nt,
                                                 qftbx::Range(1e-3, 100.0), 100))
            << "two searches at once must not be allowed";
    }

    controller.waitForComputation();

    EXPECT_NO_THROW(controller.setPlant(makePlant()));
}

TEST(BackgroundSearch, CancellingFromAnotherThreadStopsTheSearch)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    ASSERT_TRUE(controller.startLoopShaping(0.5, qftbx::nt,
                                            qftbx::Range(1e-3, 100.0), 100));
    controller.cancelComputation();
    controller.waitForComputation();

    EXPECT_FALSE(controller.isComputing());

    if (controller.lastComputationCancelled()) {
        EXPECT_FALSE(controller.lastComputationProduced());
        EXPECT_EQ(controller.loopShapingResult(), nullptr)
            << "a cancelled search must not leave half a design behind";
    } else {
        EXPECT_TRUE(controller.lastComputationProduced());
        EXPECT_NE(controller.loopShapingResult(), nullptr);
    }
}

TEST(BackgroundSearch, AFailedSearchIsReportedAndNotThrownFromTheWorker)
{
    ProjectController controller;
    controller.setPlant(makePlant());
    controller.setOmega(makeOmega());

    EXPECT_THROW(controller.startLoopShaping(0.5, qftbx::nt,
                                             qftbx::Range(1e-3, 100.0), 100),
                 qftbx::InvalidInput)
        << "a search with no boundaries is refused where the caller can see it";

    EXPECT_FALSE(controller.isComputing());
}

TEST(PipelineSteps, CompletedGrowsWithTheWalkAndIsDerived)
{
    ProjectController controller;

    EXPECT_TRUE(controller.completed().empty());

    controller.setPlant(makePlant());
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Plant));
    EXPECT_FALSE(controller.completed().has(qftbx::Step::Frequencies));

    controller.setSpecifications(makeSpecifications());
    controller.setOmega(makeOmega());
    EXPECT_EQ(controller.completed().count(), 3u);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Templates));

    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Boundaries));

    controller.setControllerStructure(makeControllerStructure());
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt,
                                              qftbx::Range(1e-3, 100.0), 100));

    EXPECT_EQ(controller.completed().count(), qftbx::kStepCount)
        << "a finished design has every step done";
}

TEST(PipelineSteps, CompletedShrinksWhenAnInputIsRepublished)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    ASSERT_TRUE(controller.completed().has(qftbx::Step::Boundaries));

    std::vector<Parameter> numerator{Parameter(2.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};
    controller.setPlant(std::make_unique<PolynomialForm>(
        std::string("P"), numerator, denominator,
        Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(0.0)));

    EXPECT_TRUE(controller.completed().has(qftbx::Step::Plant));
    EXPECT_FALSE(controller.completed().has(qftbx::Step::Templates));
    EXPECT_FALSE(controller.completed().has(qftbx::Step::Boundaries));
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Controller))
        << "the controller structure is an input, and inputs do not invalidate inputs";
}

TEST(PipelineSteps, InvalidateFromDropsExactlyTheCascade)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    controller.invalidateFrom(qftbx::Step::Boundaries);

    EXPECT_FALSE(controller.completed().has(qftbx::Step::Boundaries));
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Templates))
        << "the boundaries are computed FROM the templates, not the other way";

    controller.invalidateFrom(qftbx::Step::Templates);

    EXPECT_FALSE(controller.completed().has(qftbx::Step::Templates));
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Frequencies));
    EXPECT_TRUE(controller.completed().has(qftbx::Step::Plant));
}

TEST(PipelineSteps, ALoadedProjectAgreesWithWhatItHolds)
{
    ProjectController controller;

    const qftbx::StepSet read = controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    EXPECT_EQ(controller.completed(), read);
}

TEST(PipelineSteps, OpeningAFileReplacesTheProjectInsteadOfOverlayingIt)
{
    ProjectController controller;

    const qftbx::StepSet full = controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_EQ(full.count(), qftbx::kStepCount) << "planta1 is a finished design";
    ASSERT_NE(controller.controllerStructure(), nullptr);
    ASSERT_NE(controller.loopShapingResult(), nullptr);

    const qftbx::StepSet partial = controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/cervera.qft"));
    ASSERT_EQ(partial.count(), 2u);

    EXPECT_EQ(controller.completed(), partial)
        << "what the project holds has to be what the file carried, no more";
    EXPECT_EQ(controller.controllerStructure(), nullptr)
        << "the previous design's controller structure must not survive the open";
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
    EXPECT_TRUE(controller.templates().empty());
    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST(PipelineSteps, InvalidatingFromTheSpecificationsKeepsTheTemplates)
{
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    controller.invalidateFrom(qftbx::Step::Specifications);

    EXPECT_TRUE(controller.completed().has(qftbx::Step::Templates));
    EXPECT_FALSE(controller.completed().has(qftbx::Step::Boundaries));
}

TEST(PipelineSteps, TheUnionGettersRefuseWithoutBoundaries)
{
    ProjectController controller;

    EXPECT_THROW(controller.unionBoundaries(), qftbx::InvalidInput);
    EXPECT_THROW(controller.unionBuckets(), qftbx::InvalidInput);
}

TEST(ChangeHandler, EveryPublishAndEveryComputationAnnouncesOnce)
{
    ProjectController controller;

    int announced = 0;
    controller.setChangeHandler([&announced]() { ++announced; });

    controller.setOmega(makeOmega());
    EXPECT_EQ(announced, 1) << "publishing the frequencies said nothing";

    controller.setPlant(makePlant());
    EXPECT_EQ(announced, 2) << "publishing the plant said nothing";

    controller.setSpecifications(makeSpecifications());
    EXPECT_EQ(announced, 3);

    controller.setControllerStructure(makeControllerStructure());
    EXPECT_EQ(announced, 4);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    EXPECT_EQ(announced, 5) << "a computation is a change too";
}

TEST(ChangeHandler, LoadingAFileAnnouncesOnce)
{
    ProjectController controller;

    int announced = 0;
    controller.setChangeHandler([&announced]() { ++announced; });

    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    EXPECT_EQ(announced, 1) << "a file with seven sections announced " << announced << " changes";
}

TEST(ChangeHandler, SettingTheHandlerIsNotItselfAChange)
{
    ProjectController controller;
    controller.setOmega(makeOmega());

    int announced = 0;
    controller.setChangeHandler([&announced]() { ++announced; });
    EXPECT_EQ(announced, 0);
}
