// The pipeline walked from nothing, in the order the interface drives it.
//
// Everything that reaches the loop shaping in this suite starts from a loaded
// .qft: the golden tests, the benchmarks, the article validations. Nothing
// walked the seven stages the way a user does - publish a plant, then the
// specifications, then the frequencies, compute the templates, the
// boundaries, publish a controller structure, run the search - asserting that
// each stage produces its own artefact and that the one before it survives.
//
// It is the net for splitting ProjectController into a class per stage
// (plan 10.3): the split must not change any of this. Written against the
// CURRENT behaviour on purpose, so a difference means the split changed
// something, not that the test needs adjusting.
//
// The fixture is deliberately tiny - three frequencies, three points per
// grid, a coarse Nichols grid - because what is under test is the sequence,
// not the numbers. The numbers have their own golden tests.

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

//P(s) = kv / (a*s + 1), both coefficients uncertain so the sweep has a grid
//to walk and the templates come out as areas rather than points.
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

//K(s) = kc * (s + z) / (s + p), with the gain and both roots given as search
//boxes.
//
//Two things this test found by being written, both worth pinning. It is a
//ZeroPoleGain and not a PolynomialForm because the loop shaping refuses
//polynomial and free-form structures: the interval projection of those is not
//implemented. And the gain is "kc" and not "k" because muParserX reserves k
//as its kilo postfix operator - see StageSequence.ReservedParameterNames.
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

//A permissive constant stability bound, in LINEAR magnitude, over the whole
//frequency band.
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

} // namespace

TEST(StageSequence, TheSevenStagesWalkedFromNothing)
{
    ProjectController controller;

    // --- 1: the plant
    EXPECT_TRUE(controller.setPlant(makePlant()));
    ASSERT_NE(controller.plant(), nullptr);

    // --- 2: the specifications
    controller.setSpecifications(makeSpecifications());
    ASSERT_NE(controller.specifications(), nullptr);

    // --- 3: the design frequencies
    EXPECT_TRUE(controller.setOmega(makeOmega()));
    ASSERT_NE(controller.omega(), nullptr);
    ASSERT_EQ(controller.omega()->values()->size(), 3u);

    // --- 4: the templates. One cloud per design frequency, and the plant
    // and the frequencies are still the ones published above.
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_EQ(controller.templates().size(), 3u);
    for (const qftbx::ComplexCloud & cloud : controller.templates()) {
        EXPECT_FALSE(cloud.empty());
    }
    EXPECT_NE(controller.plant(), nullptr);
    EXPECT_NE(controller.omega(), nullptr);

    // --- 5: the boundaries, over a coarse Nichols grid.
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    ASSERT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty())
        << "computing the boundaries must not disturb the templates they came from";

    // --- 6: the controller structure. It invalidates nothing here, because
    // there is no loop shaping yet to invalidate.
    EXPECT_TRUE(controller.setControllerStructure(makeControllerStructure()));
    ASSERT_NE(controller.controllerStructure(), nullptr);
    EXPECT_NE(controller.boundaries(), nullptr);

    // --- 7: the search.
    ASSERT_TRUE(controller.computeLoopShaping(0.5, qftbx::nt,
                                              qftbx::Range(1e-3, 100.0), 100));
    ASSERT_NE(controller.loopShapingResult(), nullptr);

    LtiSystem * const designed = controller.loopShapingResult()->controller();
    ASSERT_NE(designed, nullptr);

    // The answer is a POINT of the search box, not the box.
    EXPECT_EQ(designed->gain().range().min, designed->gain().range().max);
    EXPECT_GT(designed->gain().range().min, 0.0);

    // And everything the search consumed is still standing.
    EXPECT_NE(controller.plant(), nullptr);
    EXPECT_NE(controller.boundaries(), nullptr);
    EXPECT_FALSE(controller.templates().empty());
}

TEST(StageSequence, ANullStepIsRefusedInsteadOfWipingTheProject)
{
    //There is no "remove the plant" step in the pipeline: a null publish was
    //taken as a change and wiped the step and everything computed from it,
    //which a reused dialog once did by accident. The facade refuses it now,
    //so no interface mistake can reach the project that way.
    ProjectController controller;

    EXPECT_THROW(controller.setPlant(nullptr), qftbx::InvalidInput);
    EXPECT_THROW(controller.setOmega(nullptr), qftbx::InvalidInput);
    EXPECT_THROW(controller.setControllerStructure(nullptr), qftbx::InvalidInput);
    EXPECT_THROW(controller.setSpecifications(std::nullopt), qftbx::InvalidInput);
}

TEST(StageSequence, ReservedParameterNamesAreRefusedWhenPublished)
{
    // Found by writing the walk above: muParserX reserves six single letters
    // as SI unit postfix operators - n, u, m, k, M and G - and it refuses to
    // bind a variable under any of them. "k" is the one that matters, being
    // what everybody calls a gain.
    //
    // The old failure mode was as bad as it gets: the name was accepted, the
    // plant and the boundaries computed fine, and then the search threw a
    // mup::ParserError from deep inside. That is neither a qftbx::Exception
    // nor a std::exception, and the window catches only the first, so the
    // application terminated. It is refused at publication now, which is
    // once per project and nowhere near the search.
    for (const char * reserved : {"n", "u", "m", "k", "M", "G"}) {
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
}

TEST(StageSequence, AConstantKeepsItsNumericName)
{
    // The check must not reach constants: Parameter(double) names itself with
    // the number, which is not an identifier at all, and no expression ever
    // binds it as a variable.
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
    // The boundaries are computed FROM the templates, so a new sweep voids
    // them. Every other link of the chain had a test; this one did not, and
    // it went missing the moment the publishing moved into TemplateStage:
    // ProjectController::computeTemplates used to reach the invalidation
    // through setTemplates, and delegating the publishing bypassed it. The
    // whole suite stayed green.
    ProjectController controller;

    controller.setPlant(makePlant());
    controller.setSpecifications(makeSpecifications());
    controller.setOmega(makeOmega());

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));
    ASSERT_NE(controller.boundaries(), nullptr);

    // A second sweep, with a different epsilon: the boundaries must be gone.
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 8.0), makeGrids(), false));

    EXPECT_EQ(controller.boundaries(), nullptr)
        << "boundaries computed from the previous templates must not survive a new sweep";
    EXPECT_FALSE(controller.templates().empty());
}

TEST(StageSequence, RecomputingTheBoundariesDropsTheLoopShaping)
{
    // The link one below the previous test, and it is written BEFORE the
    // boundary stage is carved out on purpose: moving the publishing into a
    // stage is exactly what broke this link for the templates, and the suite
    // did not notice. The search runs against a set of boundaries, so a new
    // set voids its answer.
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

    // A second boundary computation, on a different grid.
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 25,
                                             qftbx::Range(-40.0, 40.0), 15,
                                             -1.0, false, false));

    EXPECT_EQ(controller.loopShapingResult(), nullptr)
        << "a design found against the previous boundaries must not survive them";
    EXPECT_NE(controller.boundaries(), nullptr);
}

// --- cancellation -----------------------------------------------------------
//
// The mechanism, without the button: the interface has no way to raise this
// yet, and where the button goes is a question for a later phase. What is
// pinned here is that the search reads the flag, that giving up travels all
// the way out as qftbx::Cancelled, and that a token does not linger into the
// next run.
//
// The cross-thread part is not tested, deliberately: an std::atomic being
// visible to another thread is the standard library's contract, not this
// code's. What this code owes is a search that looks at the flag often enough
// to matter, and it looks once per node.

namespace {

//A project standing at the point where the search can be started.
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

} // namespace

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
    // The stage installs the token on EVERY run, so a cancelled one from a
    // previous attempt cannot poison the next. Without that the engine, which
    // is kept between runs, would still be holding it.
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    qftbx::CancellationToken token;
    token.cancel();

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::nt,
                                               qftbx::Range(1e-3, 100.0), 100,
                                               0, &token),
                 qftbx::Cancelled);

    // Again, with no token at all: it has to run to the end.
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

// --- the search off the calling thread --------------------------------------
//
// The worker lives in the facade and not in the interface because the facade
// owns the project data: while a search is in flight nothing may touch them,
// and the only place that can enforce that is the one holding them. These
// tests are the reason that is worth saying - they run the whole thing with
// no interface at all.

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
    // The point of the worker being here. A plant published mid-search would
    // be read by the search from another thread, and the invalidation would
    // drop the boundaries it is walking.
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    // A token cancelled up front keeps the worker alive just long enough to
    // be observed without racing on how fast the search is: the run has to
    // finish, and until it does the project is closed for business.
    ASSERT_TRUE(controller.startLoopShaping(0.5, qftbx::nt,
                                            qftbx::Range(1e-3, 100.0), 100));

    // Whether the search is still running by now is a race, so both outcomes
    // are accepted - what must NOT happen is a change going through while it
    // is in flight.
    if (controller.isComputing()) {
        EXPECT_THROW(controller.setPlant(makePlant()), qftbx::InvalidInput);
        EXPECT_THROW(controller.load(std::string("/nonexistent.qft")),
                     qftbx::InvalidInput);
        EXPECT_FALSE(controller.startLoopShaping(0.5, qftbx::nt,
                                                 qftbx::Range(1e-3, 100.0), 100))
            << "two searches at once must not be allowed";
    }

    controller.waitForComputation();

    // And it opens again afterwards.
    EXPECT_NO_THROW(controller.setPlant(makePlant()));
}

TEST(BackgroundSearch, CancellingFromAnotherThreadStopsTheSearch)
{
    // The real race, run the only way it can be run honestly: cancel while
    // the search is in flight and accept either outcome, because whether the
    // flag lands before the search finishes depends on the machine. What is
    // asserted is what must hold EITHER way - it ends, it ends exactly once,
    // and a cancelled run publishes nothing.
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
    // An exception escaping the function of an std::thread terminates the
    // process, and this search can throw four unrelated families - two of
    // which derive from neither std::exception nor each other. So the worker
    // catches everything and the outcome is asked for afterwards.
    //
    // The preconditions are the exception: they are checked on the CALLER's
    // thread, before the worker starts, so a caller sees its own mistake.
    ProjectController controller;
    controller.setPlant(makePlant());
    controller.setOmega(makeOmega());

    EXPECT_THROW(controller.startLoopShaping(0.5, qftbx::nt,
                                             qftbx::Range(1e-3, 100.0), 100),
                 qftbx::InvalidInput)
        << "a search with no boundaries is refused where the caller can see it";

    EXPECT_FALSE(controller.isComputing());
}

// --- the pipeline as data ---------------------------------------------------

TEST(PipelineSteps, CompletedGrowsWithTheWalkAndIsDerived)
{
    // completed() is computed from what the project holds, not stored, so it
    // cannot go stale - which is the whole reason for it. The window keeps
    // seven booleans saying the same thing by hand today.
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
    // Being derived is what makes this automatic: publishing a new plant
    // drops the templates and below, and completed() says so without anyone
    // having to remember to update it.
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    ASSERT_TRUE(controller.completed().has(qftbx::Step::Boundaries));

    //A different plant. makePlant() here builds one fixed system, so the
    //difference is made with the structure, which sameAs() compares whole.
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
    // One implementation of the dependency order, asked for by step instead
    // of through three functions that call each other.
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
    // The two answers have to be the same one: what load() says it read, and
    // what completed() derives from the data it left behind.
    ProjectController controller;

    const qftbx::StepSet read = controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    EXPECT_EQ(controller.completed(), read);
}

TEST(PipelineSteps, OpeningAFileReplacesTheProjectInsteadOfOverlayingIt)
{
    // load() used to publish only the steps the file carried, on top of
    // whatever the project already held. A partial file opened over a finished
    // design left a hybrid - the new plant under the old controller structure
    // and the old search result - and completed(), derived from the data,
    // reported more steps done than the file had. The file replaces the
    // project now.
    ProjectController controller;

    const qftbx::StepSet full = controller.load(
        std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_EQ(full.count(), qftbx::kStepCount) << "planta1 is a finished design";
    ASSERT_NE(controller.controllerStructure(), nullptr);
    ASSERT_NE(controller.loopShapingResult(), nullptr);

    // cervera carries a plant and the frequencies, nothing else.
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
    // The templates do not depend on the specifications, only the boundaries
    // do. The first invalidateFrom() grouped the specifications with the
    // plant and dropped the templates too - disagreeing with
    // setSpecifications(), which had it right.
    ProjectController controller;
    ASSERT_NO_FATAL_FAILURE(prepareForSearch(controller));

    controller.invalidateFrom(qftbx::Step::Specifications);

    EXPECT_TRUE(controller.completed().has(qftbx::Step::Templates));
    EXPECT_FALSE(controller.completed().has(qftbx::Step::Boundaries));
}

TEST(PipelineSteps, TheUnionGettersRefuseWithoutBoundaries)
{
    // Every other getter answers nullptr while its step is not done; these two
    // return references and cannot, so they used to dereference a null.
    ProjectController controller;

    EXPECT_THROW(controller.unionBoundaries(), qftbx::InvalidInput);
    EXPECT_THROW(controller.unionBuckets(), qftbx::InvalidInput);
}
