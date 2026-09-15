// The plane the templates' epsilon is measured in, as a property of the
// project: kept with the epsilon, saved with it (format version 3), read back
// from it, and the plane every contour computation measures in.
//
// A version-2 file carries no plane and is read as what it is: an epsilon in
// the complex plane. A file the build does not know is refused, as before.

#include <gtest/gtest.h>

#include <QTemporaryDir>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "src/app/project_controller.h"
#include "src/core/common/exception.h"
#include "src/core/math/sequences.h"
#include "src/core/templates/hull_metric.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

TEST(EpsilonMetric, AFreshProjectIsInTheComplexPlaneAndALoadedVersionTwoFileToo)
{
    ProjectController fresh;
    EXPECT_EQ(fresh.epsilonMetric().metric, HullMetric::ComplexPlane);
    EXPECT_DOUBLE_EQ(fresh.epsilonMetric().dbPerDegree, 1.0);

    ProjectController loaded;
    loaded.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    EXPECT_EQ(loaded.epsilonMetric().metric, HullMetric::ComplexPlane)
            << "a version-2 file predates the choice: the complex plane";
}

TEST(EpsilonMetric, TheMetricRoundTripsThroughTheFile)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("metric.qft").toStdString();

    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    controller.setEpsilonMetric({HullMetric::Nichols, 0.5});
    controller.save(path);

    ProjectReader reader;
    reader.load(path);
    EXPECT_EQ(reader.epsilonMetric().metric, HullMetric::Nichols);
    EXPECT_DOUBLE_EQ(reader.epsilonMetric().dbPerDegree, 0.5);

    ProjectController reloaded;
    reloaded.load(path);
    EXPECT_EQ(reloaded.epsilonMetric().metric, HullMetric::Nichols);
    EXPECT_DOUBLE_EQ(reloaded.epsilonMetric().dbPerDegree, 0.5);

    //The written file says version 3 and names the plane on the epsilon.
    std::ifstream in(path);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(text.find("version=\"3\""), std::string::npos);
    EXPECT_NE(text.find("metric=\"nichols\""), std::string::npos);
    EXPECT_NE(text.find("db-per-degree=\"0.5\""), std::string::npos);
}

TEST(EpsilonMetric, AnUnknownMetricOrVersionIsRefused)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());

    const auto write = [&](const char * name, const std::string & body) {
        const std::string path = temporary.filePath(name).toStdString();
        std::ofstream out(path);
        out << body;
        return path;
    };

    ProjectReader reader;
    EXPECT_THROW(reader.load(write("v4.qft", "<?xml version=\"1.0\"?><QFT version=\"4\"></QFT>")), qftbx::ParseError);

    const std::string badMetric =
        "<?xml version=\"1.0\"?><QFT version=\"3\"><templates><metadata>"
        "<epsilon metric=\"polar\">1 </epsilon></metadata><full size=\"1\"><re>1 </re><im>0 </im></full>"
        "</templates></QFT>";
    EXPECT_THROW(reader.load(write("metric.qft", badMetric)), qftbx::ParseError);

    const std::string badWeight =
        "<?xml version=\"1.0\"?><QFT version=\"3\"><templates><metadata>"
        "<epsilon metric=\"nichols\" db-per-degree=\"0\">1 </epsilon></metadata><full size=\"1\"><re>1 </re><im>0 </im></full>"
        "</templates></QFT>";
    EXPECT_THROW(reader.load(write("weight.qft", badWeight)), qftbx::ParseError);
}

//The contour is walked in the project's plane. On example 2 an epsilon in the
//complex plane is either far too small for the template at 0.1 rad/s or ten
//thousand times too large for the one at 100 rad/s; in the Nichols plane one
//epsilon serves all six, just above what each asks for (2.77 to 8.97
//measured), so every contour closes and none is the whole cloud.
TEST(EpsilonMetric, TheContourIsWalkedInTheProjectsPlane)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const std::size_t frequencies = controller.omega()->values()->size();

    controller.setEpsilonMetric({HullMetric::Nichols, 1.0});
    const std::vector<TemplateEngine::EpsilonProposal> proposals = controller.proposeEpsilon();
    ASSERT_EQ(proposals.size(), frequencies);
    EXPECT_NEAR(proposals.back().connected, 2.766, 0.03) << "what w = 100 asks for, in degrees and dB";
    for (const auto & p : proposals) {
        EXPECT_TRUE(p.closes);
        EXPECT_GE(p.epsilon, p.connected);
    }

    //Just above the largest proposal: every template connected, none coarse.
    double largest = 0.0;
    for (const auto & p : proposals) largest = std::max(largest, p.epsilon);
    const CloudSet & contours = controller.recomputeContour(std::vector<double>(frequencies, largest * 1.05));
    ASSERT_EQ(contours.size(), frequencies);
    for (std::size_t i = 0; i < frequencies; ++i) {
        EXPECT_GT(contours[i].size(), 4u) << "frequency " << i;
        EXPECT_LT(contours[i].size(), controller.templates()[i].size()) << "frequency " << i;
    }
}

//The proposal the templates dialog opens with is made BEFORE any template
//exists: from a sweep over the grids as the dialog holds them. It has to
//agree with what the same sweep, once computed, asks for, and it must not
//publish anything or disturb the templates the project already has.
TEST(EpsilonMetric, TheProposalBeforeComputingMatchesTheOneAfter)
{
    ProjectController controller;
    controller.load(std::string(QFTBX_TEST_DATA_DIR "/qft_toolbox_ex2.qft"));
    const std::size_t storedPoints = controller.templates().at(0).size();

    qftbx::ParameterGrids grids;
    for (const Parameter & p : controller.plant()->numerator()) {
        if (p.isUncertain()) grids[p.name()] = qftbx::math::linspace(p.range().min, p.range().max, 7);
    }
    for (const Parameter & p : controller.plant()->denominator()) {
        if (p.isUncertain()) grids[p.name()] = qftbx::math::linspace(p.range().min, p.range().max, 7);
    }
    for (const Parameter * p : {&controller.plant()->gain(), &controller.plant()->delay()}) {
        grids[p->name()] = p->isUncertain()
                               ? qftbx::math::linspace(p->range().min, p->range().max, 7)
                               : std::vector<double>(1, p->nominal());
    }

    qftbx::EpsilonMetric nichols;
    nichols.metric = qftbx::HullMetric::Nichols;
    nichols.dbPerDegree = 1.0;

    const std::vector<TemplateEngine::EpsilonProposal> before = controller.proposeEpsilon(grids, nichols);
    ASSERT_EQ(before.size(), 6u);
    EXPECT_EQ(controller.templates().at(0).size(), storedPoints)
        << "proposing an epsilon replaced the project's templates";

    //Each template walked at exactly the epsilon it asked for: the walk has
    //to close, strictly, everywhere.
    std::vector<double> epsilon;
    for (const TemplateEngine::EpsilonProposal & p : before) epsilon.push_back(p.epsilon);
    controller.setEpsilonMetric(nichols);
    ASSERT_TRUE(controller.computeTemplates(epsilon, grids, false));
    EXPECT_EQ(controller.templates().at(0).size(), 49u) << "two uncertain parameters at seven points each";
    const std::vector<TemplateEngine::EpsilonProposal> after = controller.proposeEpsilon();
    ASSERT_EQ(after.size(), 6u);
    for (std::size_t i = 0; i < 6; ++i) {
        EXPECT_TRUE(before[i].closes) << "frequency " << i;
        EXPECT_DOUBLE_EQ(before[i].epsilon, after[i].epsilon) << "frequency " << i;
        EXPECT_DOUBLE_EQ(before[i].connected, after[i].connected) << "frequency " << i;
        EXPECT_DOUBLE_EQ(before[i].diameter, after[i].diameter) << "frequency " << i;
        //The contour walked at the proposed epsilon is a proper contour:
        //neither the whole cloud nor a handful of points.
        EXPECT_LT(controller.contour()[i].size(), 49u) << "frequency " << i;
        EXPECT_GT(controller.contour()[i].size(), 4u) << "frequency " << i;
    }
}
