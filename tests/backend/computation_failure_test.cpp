/**
 * @file
 * @brief Tests of what a computation that throws leaves behind.
 *
 * When templates, boundaries or loop shaping refuse their inputs the
 * project must stay coherent, holding neither half an artefact nor one
 * that belongs to inputs since replaced, and usable, so that correcting
 * the input and computing again works. A missing sweep grid is the
 * deterministic way to make the template computation refuse; a step whose
 * inputs were dropped must say so rather than run on nothing. Under
 * LeakSanitizer the cases also pin that a throw frees what it had built.
 */

#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/specifications/specification_record.h"
#include <gtest/gtest.h>

#include <string>

#include <vector>

#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/app/project_controller.h"
#include "src/core/math/range.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"

using namespace qftbx;

namespace {

std::unique_ptr<LtiSystem> makePlant()
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    return std::make_unique<PolynomialForm>(std::string("failing"), numerator, denominator,
                              Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
                              Parameter(0.0));
}

std::unique_ptr<Omega> makeOmega()
{
    return std::make_unique<Omega>(0.1, 10.0, 3, qftbx::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

qftbx::ParameterGrids gridsMissing(const std::string & name)
{
    qftbx::ParameterGrids grids;
    if (name != std::string("a")) {
        grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    }
    if (name != std::string("kv")) {
        grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
    }
    return grids;
}

class FailedComputation : public ::testing::Test
{
protected:
    void SetUp() override
    {
        controller.setPlant(makePlant());
        controller.setOmega(makeOmega());
    }

    ProjectController controller;
};

TEST_F(FailedComputation, AMissingSweepGridIsReportedAndLeavesNoTemplates)
{
    const std::vector<double> epsilon(3, 10.0);

    EXPECT_THROW(controller.computeTemplates(epsilon, gridsMissing(std::string("kv")), false),
                 qftbx::InvalidInput);

    EXPECT_TRUE(controller.templates().empty())
        << "a failed computation left templates behind";
    EXPECT_TRUE(controller.contour().empty());
    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST_F(FailedComputation, TheProjectStillWorksAfterAFailedComputation)
{
    const std::vector<double> badEpsilon(3, 10.0);
    const qftbx::ParameterGrids badGrids = gridsMissing(std::string("a"));

    EXPECT_THROW(controller.computeTemplates(badEpsilon, badGrids, false),
                 qftbx::InvalidInput);

    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids, false))
        << "the project never recovered from the earlier failure";
    EXPECT_FALSE(controller.templates().empty());

}

TEST_F(FailedComputation, BoundariesRefuseWithoutTheTemplatesInsteadOfCrashing)
{
    controller.setSpecifications(qftbx::SpecificationRecords());

    ASSERT_TRUE(controller.templates().empty());

    EXPECT_THROW(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 361,
                                             qftbx::Range(-60.0, 60.0), 121, 0.0,
                                             false, false),
                 qftbx::InvalidInput);

    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST_F(FailedComputation, LoopShapingRefusesWithoutItsInputs)
{
    EXPECT_THROW(controller.computeLoopShaping(0.01, qftbx::nt, qftbx::Range(0.1, 100.0), 50, 0),
                 qftbx::InvalidInput);

    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(FailedComputation, TemplatesRefuseWithoutAPlantOrFrequencies)
{
    const std::vector<double> epsilon(1, 10.0);
    qftbx::ParameterGrids grids;

    ProjectController empty;
    EXPECT_THROW(empty.computeTemplates(epsilon, grids, false), qftbx::InvalidInput);

    ProjectController plantOnly;
    plantOnly.setPlant(makePlant());
    EXPECT_THROW(plantOnly.computeTemplates(epsilon, {}, false), qftbx::InvalidInput);
}

}
