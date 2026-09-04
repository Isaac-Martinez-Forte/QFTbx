// What a FAILED computation leaves behind.
//
// The exception paths of the three computations had no tests at all, and they
// are where every leak of this refactor has come from. Two things have to hold
// when a computation throws:
//
//   - the project stays COHERENT: it must not be left holding half of an
//     artefact, nor an artefact belonging to inputs that have since changed;
//   - the project stays USABLE: fixing the input and computing again has to
//     work, rather than leaving the user with a project that refuses forever.
//
// Run under LeakSanitizer these also pin that a throw frees what it had built:
// nobody owns the clouds and the contours until the computation returns.

#include "src/core/loopshaping/loop_shaping_types.h"
#include "src/core/specifications/specification_record.h"
#include <gtest/gtest.h>

#include <string>

#include <vector>


#include "src/core/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/project_controller.h"
#include "src/core/range.h"
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

//Every uncertain parameter needs a sweep grid; leaving one out is the
//deterministic way to make the template computation refuse.
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

    //Same project, now with every grid: the failure must not have poisoned it.
    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);

    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0), grids, false))
        << "the project never recovered from the earlier failure";
    EXPECT_FALSE(controller.templates().empty());

}

TEST_F(FailedComputation, BoundariesRefuseWithoutTheTemplatesInsteadOfCrashing)
{
    //Publishing an input drops what was computed from the old one, so a step
    //whose inputs were invalidated has to SAY so: before this it walked a null
    //pointer into the boundary engine.
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
    //A refusal used to leak the epsilon the caller had allocated for it:
    //the preconditions fire before the store takes it. By value there is
    //nothing to leak on any path.
    const std::vector<double> epsilon(1, 10.0);
    qftbx::ParameterGrids grids;

    ProjectController empty;
    EXPECT_THROW(empty.computeTemplates(epsilon, grids, false), qftbx::InvalidInput);

    ProjectController plantOnly;
    plantOnly.setPlant(makePlant());
    EXPECT_THROW(plantOnly.computeTemplates(epsilon, {}, false), qftbx::InvalidInput);
}

} // namespace
