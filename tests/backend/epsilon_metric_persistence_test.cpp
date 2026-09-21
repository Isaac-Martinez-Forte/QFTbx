/**
 * @file
 * @brief Tests of the plane the templates' epsilon is measured in.
 *
 * The metric is a property of the project: kept with the epsilon, saved with
 * it among the settings of a version 4 file, read back, and used by every
 * contour computation. A file ported from version 2 carries no plane and is
 * read as an epsilon in the complex plane; a file of a version the build does
 * not read is refused naming both versions, and an epsilon stored among the
 * results rather than the settings is still read. On QFT toolbox example 2
 * one Nichols-plane epsilon serves all six frequencies, just above what each
 * asks for (2.77 to 8.97), so every contour closes and none is the whole
 * cloud; and the proposal made from the sweep grids before any template
 * exists must equal the one made afterwards, leaving the templates untouched.
 */

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
            << "a file ported from version 2 predates the choice: the complex plane";
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

    std::ifstream in(path);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(text.find("version=\"4\""), std::string::npos);
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
    const ProjectReader::Loaded empty =
            reader.load(write("empty.qft", "<?xml version=\"1.0\"?><QFT version=\"4\"></QFT>"));
    EXPECT_EQ(empty.steps.count(), 0u);

    const std::string badMetric =
        "<?xml version=\"1.0\"?><QFT version=\"4\"><inputs/><settings><templates>"
        "<epsilon metric=\"polar\">1 </epsilon></templates></settings>"
        "<results><templates><full size=\"1\"><re>1 </re><im>0 </im></full>"
        "</templates></results></QFT>";
    EXPECT_THROW(reader.load(write("metric.qft", badMetric)), qftbx::ParseError);

    const std::string badWeight =
        "<?xml version=\"1.0\"?><QFT version=\"4\"><inputs/><settings><templates>"
        "<epsilon metric=\"nichols\" db-per-degree=\"0\">1 </epsilon></templates></settings>"
        "<results><templates><full size=\"1\"><re>1 </re><im>0 </im></full>"
        "</templates></results></QFT>";
    EXPECT_THROW(reader.load(write("weight.qft", badWeight)), qftbx::ParseError);

    try {
        reader.load(write("old.qft", "<?xml version=\"1.0\"?><QFT version=\"2\"></QFT>"));
        ADD_FAILURE() << "a version-2 file was read";
    } catch (const qftbx::ParseError & refused) {
        const std::string said = refused.what();
        EXPECT_NE(said.find("found 2"), std::string::npos) << said;
        EXPECT_NE(said.find("version 4"), std::string::npos) << said;
        EXPECT_EQ(said.find('%'), std::string::npos) << "the message keeps a placeholder: " << said;
    }
}

TEST(EpsilonMetric, AFileThatKeptItsEpsilonAmongTheResultsIsStillRead)
{
    QTemporaryDir temporary;
    ASSERT_TRUE(temporary.isValid());
    const std::string path = temporary.filePath("among_results.qft").toStdString();

    std::ofstream out(path);
    out << "<?xml version=\"1.0\"?><QFT version=\"4\"><inputs/><results><templates><metadata>"
           "<epsilon metric=\"nichols\" db-per-degree=\"2\">3 4 </epsilon></metadata>"
           "<full size=\"1\"><re>1 </re><im>0 </im></full></templates></results></QFT>";
    out.close();

    ProjectReader reader;
    reader.load(path);

    ASSERT_NE(reader.epsilon(), nullptr);
    ASSERT_EQ(reader.epsilon()->size(), 2u);
    EXPECT_DOUBLE_EQ(reader.epsilon()->at(0), 3.0);
    EXPECT_EQ(reader.epsilonMetric().metric, HullMetric::Nichols);
    EXPECT_DOUBLE_EQ(reader.epsilonMetric().dbPerDegree, 2.0);
}

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

    double largest = 0.0;
    for (const auto & p : proposals) largest = std::max(largest, p.epsilon);
    const CloudSet & contours = controller.recomputeContour(std::vector<double>(frequencies, largest * 1.05));
    ASSERT_EQ(contours.size(), frequencies);
    for (std::size_t i = 0; i < frequencies; ++i) {
        EXPECT_GT(contours[i].size(), 4u) << "frequency " << i;
        EXPECT_LT(contours[i].size(), controller.templates()[i].size()) << "frequency " << i;
    }
}

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
        EXPECT_LT(controller.contour()[i].size(), 49u) << "frequency " << i;
        EXPECT_GT(controller.contour()[i].size(), 4u) << "frequency " << i;
    }
}
