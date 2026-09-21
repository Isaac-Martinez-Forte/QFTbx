/**
 * @file
 * @brief What a new input drops from the project, and what it must keep.
 *
 * Everything the sweeps and the search compute is a function of the inputs
 * below it, so publishing a new plant or a new frequency set drops the
 * templates and everything after them, while republishing an equal one keeps
 * them: the dialog hands over a fresh object every time it is accepted, and
 * comparison is by value. New specifications keep the templates and drop the
 * boundaries; a new controller structure drops the result alone.
 * Over-invalidating would throw away work the user still has and break
 * loading, which assigns in dependency order, so both directions are pinned,
 * and the project must be recomputable afterwards.
 */

#include "src/core/specifications/specification_record.h"
#include <gtest/gtest.h>

#include <string>

#include <vector>

#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/app/project_controller.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/math/range.h"

using namespace qftbx;

namespace {

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

qftbx::ParameterGrids makeGrids()
{
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
    return grids;
}

std::unique_ptr<Omega> makeOmega()
{
    return std::make_unique<Omega>(0.1, 10.0, 3, qftbx::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

std::unique_ptr<Omega> makeOtherOmega()
{
    return std::make_unique<Omega>(0.01, 100.0, 4, qftbx::logspace(-2.0, 2.0, 4), Omega::LogSpace);
}

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
    EXPECT_TRUE(controller.setOmega(makeOtherOmega()));

    EXPECT_TRUE(controller.templates().empty());
    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST_F(Staleness, RepublishingTheSameFrequenciesKeepsTheTemplates)
{
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
    EXPECT_TRUE(controller.setPlant(makePlant(std::string("another"))));

    EXPECT_TRUE(controller.templates().empty());
}

TEST_F(Staleness, PublishingAPlantTakesItsOwnership)
{
    std::unique_ptr<LtiSystem> published = makePlant(std::string("published"));
    LtiSystem * const handedOver = published.get();

    controller.setPlant(std::move(published));

    EXPECT_EQ(controller.plant(), handedOver);
    EXPECT_TRUE(controller.templates().empty())
        << "a published plant must drop what was computed from the old one";
}

TEST_F(Staleness, NewSpecificationsKeepTheTemplatesAndDropTheBoundaries)
{
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
    controller.setPlant(makePlant(std::string("third")));
    ASSERT_TRUE(controller.templates().empty());

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), makeGrids(), false));

    EXPECT_FALSE(controller.templates().empty())
        << "the project could not be brought back to a computed state";
}

}
