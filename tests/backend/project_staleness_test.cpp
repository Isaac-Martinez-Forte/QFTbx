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

#include <gtest/gtest.h>

#include <QHash>
#include <QString>
#include <QVector>

#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/project_controller.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/range.h"

namespace {

//P(s) = kv / (a*s + 1), with both coefficients uncertain so the sweep has a
//grid to walk.
LtiSystem * makePlant(const QString & name)
{
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(QStringLiteral("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    return new PolynomialForm(name, numerator, denominator,
                              Parameter(QStringLiteral("kv"), qftbx::Range(1.0, 2.0), 1.5),
                              Parameter(0.0));
}

QHash<QString, QVector<qreal> *> * makeGrids()
{
    auto * grids = new QHash<QString, QVector<qreal> *>();
    grids->insert(QStringLiteral("a"), tools::linspace(1.0, 2.0, 3));
    grids->insert(QStringLiteral("kv"), tools::linspace(1.0, 2.0, 3));
    return grids;
}

Omega * makeOmega()
{
    return new Omega(0.1, 10.0, 3, tools::logspace(-1.0, 1.0, 3), Omega::LogSpace);
}

//A project with plant, frequencies and computed templates.
class Staleness : public ::testing::Test
{
protected:
    void SetUp() override
    {
        controller.setPlant(makePlant(QStringLiteral("first")));
        controller.setOmega(makeOmega());

        ASSERT_TRUE(controller.computeTemplates(new QVector<qreal>(3, 10.0), makeGrids(), false));
        ASSERT_NE(controller.templates(), nullptr);
    }

    ProjectController controller;
};

TEST_F(Staleness, ANewPlantDropsTheTemplatesComputedForTheOldOne)
{
    controller.setPlant(makePlant(QStringLiteral("second")));

    EXPECT_EQ(controller.templates(), nullptr)
        << "the templates of the previous plant survived; computeBoundaries "
           "would have combined them with the new plant";
    EXPECT_EQ(controller.contour(), nullptr);
    EXPECT_EQ(controller.boundaries(), nullptr);
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(Staleness, NewFrequenciesDropTheTemplatesToo)
{
    //The templates are one cloud per design frequency: a different frequency
    //set does not merely invalidate them, it changes what they even mean.
    controller.setOmega(makeOmega());

    EXPECT_EQ(controller.templates(), nullptr);
    EXPECT_EQ(controller.boundaries(), nullptr);
}

TEST_F(Staleness, RepublishingTheSamePlantChangesNothing)
{
    //Identity is not a change. Dropping a finished design because the same
    //object was handed over twice would be worse than the bug this fixes.
    LtiSystem * same = controller.plant();
    auto * templatesBefore = controller.templates();

    controller.setPlant(same);

    EXPECT_EQ(controller.plant(), same);
    EXPECT_EQ(controller.templates(), templatesBefore)
        << "re-publishing the same plant threw the templates away";
}

TEST_F(Staleness, NewSpecificationsKeepTheTemplatesAndDropTheBoundaries)
{
    //The templates are a property of the plant and the frequencies alone.
    //Dropping them here would also break load(), which sets the
    //specifications BEFORE the templates.
    auto * templatesBefore = controller.templates();

    auto * records = new QVector<qftbx::SpecificationRecord *>();
    for (int i = 0; i < 7; i++) {
        records->append(new qftbx::SpecificationRecord());
    }
    controller.setSpecifications(records);

    EXPECT_EQ(controller.templates(), templatesBefore)
        << "the specifications do not determine the templates";
    EXPECT_EQ(controller.boundaries(), nullptr);
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(Staleness, ANewControllerStructureKeepsEverythingButTheResult)
{
    auto * templatesBefore = controller.templates();

    controller.setControllerStructure(makePlant(QStringLiteral("structure")));

    EXPECT_EQ(controller.templates(), templatesBefore)
        << "the controller structure does not determine the templates";
    EXPECT_EQ(controller.loopShapingResult(), nullptr);
}

TEST_F(Staleness, TheTemplatesCanBeRecomputedAfterTheirInputsChange)
{
    //Invalidation must leave the project usable, not stuck: the point is to
    //force a recomputation, not to forbid one.
    controller.setPlant(makePlant(QStringLiteral("third")));
    ASSERT_EQ(controller.templates(), nullptr);

    ASSERT_TRUE(controller.computeTemplates(new QVector<qreal>(3, 10.0), makeGrids(), false));

    EXPECT_NE(controller.templates(), nullptr)
        << "the project could not be brought back to a computed state";
}

} // namespace
