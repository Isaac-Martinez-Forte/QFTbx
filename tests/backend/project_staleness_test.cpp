// Whatever the sweeps and the search COMPUTE is a function of the inputs
// below it. Nothing used to enforce that: publishing a new plant on top of a
// finished design left the old templates in place, and computeBoundaries()
// then combined the NEW plant with the OLD templates and produced boundaries
// for a system that never existed. Silently, because every pointer involved
// was perfectly valid.
//
// These tests pin the dependency chain, in both directions: what a new input
// must drop, and - just as important - what it must NOT drop, because
// over-invalidating would throw away work the user still has, and would break
// load(), which assigns in dependency order.

#include "src/core/specifications/specification_record.h"
#include <gtest/gtest.h>

#include <string>

#include <vector>


#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/project_controller.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/range.h"

namespace {

//P(s) = kv / (a*s + 1), with both coefficients uncertain so the sweep has a
//grid to walk.
std::unique_ptr<LtiSystem> makePlant(const std::string & name)
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    return std::make_unique<PolynomialForm>(name, numerator, denominator,
                              Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
                              Parameter(0.0));
}

//By value: no walking the map to delete each vector, and no question of who
//owns it on a throw path. That is what the conversion buys.
qftbx::ParameterGrids makeGrids()
{
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
    return grids;
}

std::unique_ptr<Omega> makeOmega()
{
    return std::make_unique<Omega>(0.1, 10.0, 3, tools::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

//A different frequency set: four values over two more decades.
std::unique_ptr<Omega> makeOtherOmega()
{
    return std::make_unique<Omega>(0.01, 100.0, 4, tools::logspace(-2.0, 2.0, 4), Omega::LogSpace);
}

//A project with plant, frequencies and computed templates.
class Staleness : public ::testing::Test
{
protected:
    void SetUp() override
    {
        controller.setPlant(makePlant(std::string("first")));
        controller.setOmega(makeOmega());

        ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));
        ASSERT_FALSE(controller.templates().empty());
    }

    ProjectController controller;
};

TEST_F(Staleness, ANewPlantDropsTheTemplatesComputedForTheOldOne)
{
    controller.setPlant(makePlant(std::string("second")));

    EXPECT_TRUE(controller.templates().empty())
        << "the templates of the previous plant survived; computeBoundaries "
           "would have combined them with the new plant";
    EXPECT_TRUE(controller.contour().empty());
    EXPECT_EQ(controller.boundaries(), nullptr);
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(Staleness, NewFrequenciesDropTheTemplatesToo)
{
    //The templates are one cloud per design frequency: a different frequency
    //set does not merely invalidate them, it changes what they even mean.
    //
    //This used to publish makeOmega() - the set the fixture had ALREADY
    //published - so it asserted on republishing, not on a new set, while its
    //name claimed otherwise. It passed because the facade invalidated
    //unconditionally.
    EXPECT_TRUE(controller.setOmega(makeOtherOmega()));

    EXPECT_TRUE(controller.templates().empty());
    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST_F(Staleness, RepublishingTheSameFrequenciesKeepsTheTemplates)
{
    //The other direction, which is the point of comparing by value: the
    //dialog hands over a freshly built Omega every time it is accepted, so
    //without this the user pressing OK without an edit paid for the whole
    //sweep again.
    EXPECT_FALSE(controller.setOmega(makeOmega()));

    EXPECT_FALSE(controller.templates().empty());
}

TEST_F(Staleness, RepublishingTheSamePlantKeepsTheTemplates)
{
    EXPECT_FALSE(controller.setPlant(makePlant(std::string("first"))));

    EXPECT_FALSE(controller.templates().empty());
}

TEST_F(Staleness, ADifferentPlantDropsTheTemplates)
{
    //Same coefficients, different NAME: still a different plant. sameAs is
    //deliberately total, because a wrong "equal" is the silent defect and a
    //wrong "different" only costs a recomputation.
    EXPECT_TRUE(controller.setPlant(makePlant(std::string("another"))));

    EXPECT_TRUE(controller.templates().empty());
}

TEST_F(Staleness, PublishingAPlantTakesItsOwnership)
{
    //This used to be RepublishingTheSamePlantChangesNothing: the facade
    //compared the incoming pointer with the stored one and returned early,
    //so that handing the same object over twice would not throw away a
    //finished design. The store takes the ownership now, so a caller cannot
    //hand back what it has already given, and the guard would have been
    //worse than its absence (it returned while still owning the object,
    //destroying the plant the store was pointing at).
    std::unique_ptr<LtiSystem> published = makePlant(std::string("published"));
    LtiSystem * const handedOver = published.get();

    controller.setPlant(std::move(published));

    EXPECT_EQ(controller.plant(), handedOver);
    EXPECT_TRUE(controller.templates().empty())
        << "a published plant must drop what was computed from the old one";
}

TEST_F(Staleness, NewSpecificationsKeepTheTemplatesAndDropTheBoundaries)
{
    //The templates are a property of the plant and the frequencies alone.
    //Dropping them here would also break load(), which sets the
    //specifications BEFORE the templates.
    const qftbx::CloudSet templatesBefore = controller.templates();

    controller.setSpecifications(qftbx::SpecificationRecords());

    EXPECT_EQ(controller.templates(), templatesBefore)
        << "the specifications do not determine the templates";
    EXPECT_EQ(controller.boundaries(), nullptr);
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(Staleness, ANewControllerStructureKeepsEverythingButTheResult)
{
    const qftbx::CloudSet templatesBefore = controller.templates();

    controller.setControllerStructure(makePlant(std::string("structure")));

    EXPECT_EQ(controller.templates(), templatesBefore)
        << "the controller structure does not determine the templates";
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(Staleness, TheTemplatesCanBeRecomputedAfterTheirInputsChange)
{
    //Invalidation must leave the project usable, not stuck: the point is to
    //force a recomputation, not to forbid one.
    controller.setPlant(makePlant(std::string("third")));
    ASSERT_TRUE(controller.templates().empty());

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));

    EXPECT_FALSE(controller.templates().empty())
        << "the project could not be brought back to a computed state";
}

} // namespace
