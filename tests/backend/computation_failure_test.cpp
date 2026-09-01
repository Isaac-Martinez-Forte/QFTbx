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

#include <gtest/gtest.h>

#include <QHash>
#include <QString>
#include <QVector>

#include "src/core/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/project_controller.h"
#include "src/core/range.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"

namespace {

LtiSystem * makePlant()
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(QStringLiteral("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    return new PolynomialForm(QStringLiteral("failing"), numerator, denominator,
                              Parameter(QStringLiteral("kv"), qftbx::Range(1.0, 2.0), 1.5),
                              Parameter(0.0));
}

Omega * makeOmega()
{
    return new Omega(0.1, 10.0, 3, tools::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

//Every uncertain parameter needs a sweep grid; leaving one out is the
//deterministic way to make the template computation refuse.
qftbx::ParameterGrids gridsMissing(const QString & name)
{
    qftbx::ParameterGrids grids;
    if (name != QStringLiteral("a")) {
        grids[QStringLiteral("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    }
    if (name != QStringLiteral("kv")) {
        grids[QStringLiteral("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
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
    auto * epsilon = new QVector<qreal>(3, 10.0);

    EXPECT_THROW(controller.computeTemplates(epsilon, gridsMissing(QStringLiteral("kv")), false),
                 qftbx::InvalidInput);

    EXPECT_EQ(controller.templates(), nullptr)
        << "a failed computation left templates behind";
    EXPECT_EQ(controller.contour(), nullptr);
    EXPECT_EQ(controller.boundaries(), nullptr);

    delete epsilon;
}

TEST_F(FailedComputation, TheProjectStillWorksAfterAFailedComputation)
{
    auto * badEpsilon = new QVector<qreal>(3, 10.0);
    const qftbx::ParameterGrids badGrids = gridsMissing(QStringLiteral("a"));

    EXPECT_THROW(controller.computeTemplates(badEpsilon, badGrids, false),
                 qftbx::InvalidInput);

    delete badEpsilon;

    //Same project, now with every grid: the failure must not have poisoned it.
    qftbx::ParameterGrids grids;
    grids[QStringLiteral("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[QStringLiteral("kv")] = qftbx::math::linspace(1.0, 2.0, 3);

    ASSERT_TRUE(controller.computeTemplates(new QVector<qreal>(3, 10.0), grids, false))
        << "the project never recovered from the earlier failure";
    EXPECT_NE(controller.templates(), nullptr);

}

TEST_F(FailedComputation, BoundariesRefuseWithoutTheTemplatesInsteadOfCrashing)
{
    //Publishing an input drops what was computed from the old one, so a step
    //whose inputs were invalidated has to SAY so: before this it walked a null
    //pointer into the boundary engine.
    auto * records = new QVector<qftbx::SpecificationRecord *>();
    for (int i = 0; i < 7; i++) {
        records->append(new qftbx::SpecificationRecord());
    }
    controller.setSpecifications(records);

    ASSERT_EQ(controller.templates(), nullptr);

    EXPECT_THROW(controller.computeBoundaries(QPointF(-360.0, 0.0), 361,
                                             QPointF(-60.0, 60.0), 121, 0.0,
                                             false, false),
                 qftbx::InvalidInput);

    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST_F(FailedComputation, LoopShapingRefusesWithoutItsInputs)
{
    EXPECT_THROW(controller.computeLoopShaping(0.01, tools::nt, QPointF(0.1, 100.0), 50, 0),
                 qftbx::InvalidInput);

    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(FailedComputation, TemplatesRefuseWithoutAPlantOrFrequencies)
{
    //A refusal takes neither the epsilon nor the grids, so this test owns
    //them - which is the point: the precondition fires before anything is
    //handed over.
    auto * epsilon = new QVector<qreal>(1, 10.0);
    qftbx::ParameterGrids grids;

    ProjectController empty;
    EXPECT_THROW(empty.computeTemplates(epsilon, grids, false), qftbx::InvalidInput);

    ProjectController plantOnly;
    plantOnly.setPlant(makePlant());
    EXPECT_THROW(plantOnly.computeTemplates(epsilon, {}, false), qftbx::InvalidInput);

    delete epsilon;
}

} // namespace
