/**
 * @file
 * @brief The sweep the templates came from, and the family verdict, survive the file.
 *
 * Computing the templates records the grids on the project; saving writes
 * them under the templates and loading brings them back equal, so a project
 * opened from a file can still be checked plant by plant. The verifier's
 * family verdict on a result is written as three attributes, or as the
 * reason it was not made, and reads back the same. A fixture from before
 * the record opens with an empty sweep.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <QTemporaryDir>

#include "src/app/project_controller.h"
#include "src/core/math/sequences.h"

using namespace qftbx;

TEST(SweepRecord, TheGridsSurviveSaveAndLoad)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("sweep.qft").toStdString();

    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    EXPECT_TRUE(project.sweepGrids().empty()) << "a fixture from before the record carries no sweep";

    ParameterGrids grids;
    for (LtiSystem * plant = project.plant(); const Parameter & p : plant->denominator()) {
        if (p.isUncertain()) grids[p.name()] = math::linspace(p.range().min, p.range().max, 3);
    }
    if (project.plant()->gain().isUncertain()) {
        const Parameter & k = project.plant()->gain();
        grids[k.name()] = math::linspace(k.range().min, k.range().max, 4);
    }
    ASSERT_FALSE(grids.empty());

    ASSERT_TRUE(project.computeTemplates(*project.epsilon(), grids, false));
    EXPECT_EQ(project.sweepGrids(), grids);

    project.save(path);

    ProjectController reloaded;
    reloaded.load(path);
    EXPECT_EQ(reloaded.sweepGrids(), grids);
    EXPECT_EQ(reloaded.templates().size(), project.templates().size());
}

TEST(SweepRecord, TheFamilyVerdictSurvivesSaveAndLoad)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("verdict.qft").toStdString();

    ProjectController project;
    project.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));
    ASSERT_NE(project.loopShapingResult(), nullptr);

    SpecificationCheck check;
    check.worstExcessDb = -0.25;
    check.family.checked = true;
    check.family.notChecked = FamilyStability::NotChecked::No;
    check.family.members = 625;
    check.family.unstableMembers = 3;
    check.family.worstRealPart = 0.125;
    project.loopShapingResult()->setCheck(check);
    EXPECT_FALSE(check.satisfied());
    project.save(path);

    ProjectController reloaded;
    reloaded.load(path);
    ASSERT_NE(reloaded.loopShapingResult(), nullptr);
    ASSERT_TRUE(reloaded.loopShapingResult()->check().has_value());
    const SpecificationCheck & back = *reloaded.loopShapingResult()->check();
    EXPECT_DOUBLE_EQ(back.worstExcessDb, -0.25);
    EXPECT_TRUE(back.family.checked);
    EXPECT_EQ(back.family.members, 625u);
    EXPECT_EQ(back.family.unstableMembers, 3u);
    EXPECT_DOUBLE_EQ(back.family.worstRealPart, 0.125);
    EXPECT_FALSE(back.satisfied());

    SpecificationCheck unchecked;
    unchecked.worstExcessDb = -0.25;
    unchecked.family.notChecked = FamilyStability::NotChecked::Delay;
    project.loopShapingResult()->setCheck(unchecked);
    project.save(path);
    ProjectController again;
    again.load(path);
    ASSERT_TRUE(again.loopShapingResult()->check().has_value());
    EXPECT_FALSE(again.loopShapingResult()->check()->family.checked);
    EXPECT_EQ(again.loopShapingResult()->check()->family.notChecked, FamilyStability::NotChecked::Delay);
    EXPECT_TRUE(again.loopShapingResult()->check()->satisfied());
}
