// The settings reader, and mostly what it REFUSES.
//
// A configuration file is user input arriving through a path nobody checks, so
// the interesting cases are the malformed ones. The rule this file pins above
// all others: a value that does not parse never becomes zero. Silently turning
// bad input into a plausible number is the defect this project has spent the
// most time removing - QString::number, std::to_string, QLineEdit::toDouble
// and muParserX all did some version of it - and a settings file would be the
// easiest place yet to reintroduce it, because nobody looks at a file that
// loads without complaining.

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "src/core/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/core/project_controller.h"
#include "src/core/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include <vector>
#include "src/core/settings.h"

namespace {

class SettingsFile : public ::testing::Test
{
protected:
    void TearDown() override
    {
        if (!m_path.empty()) {
            std::remove(m_path.c_str());
        }
    }

    //Writes a settings file and answers with its path.
    std::string written(const std::string & content)
    {
        m_path = std::string(QFTBX_TEST_DATA_DIR "/../settings_under_test.conf");

        std::ofstream file(m_path);
        file << content;
        file.close();

        return m_path;
    }

private:
    std::string m_path;
};

} // namespace

TEST_F(SettingsFile, TheDefaultsStandOnTheirOwn)
{
    //The compiled values, with no file involved. They are the contract: the
    //program has to run without a settings file at all.
    const qftbx::Settings settings;

    EXPECT_EQ(settings.limits.maxGridCells, 10000000);
    EXPECT_EQ(settings.limits.maxTemplatePoints, 1.0e6);
    EXPECT_EQ(settings.limits.maxFrequencyCount, 1000000);
    EXPECT_EQ(settings.limits.maxMagnitude, 1.0e12);
    EXPECT_EQ(settings.search.maxLiveNodes, 32000000u);
    EXPECT_TRUE(settings.source.empty());
    EXPECT_TRUE(settings.unknownKeys.empty());
}

TEST_F(SettingsFile, SectionsGroupTheKeys)
{
    const qftbx::Settings settings = qftbx::readSettings(written(
        "# A comment, and a blank line after it\n"
        "\n"
        "[limits]\n"
        "max-grid-cells = 250000\n"
        "max-frequency-count = 500   ; a trailing comment\n"
        "\n"
        "[search]\n"
        "max-live-nodes = 1000\n"));

    EXPECT_EQ(settings.limits.maxGridCells, 250000);
    EXPECT_EQ(settings.limits.maxFrequencyCount, 500);
    EXPECT_EQ(settings.search.maxLiveNodes, 1000u);

    //Untouched keys keep their compiled value: a file says what to CHANGE.
    EXPECT_EQ(settings.limits.maxMagnitude, 1.0e12);

    EXPECT_FALSE(settings.source.empty()) << "it has to say which file it read";
}

TEST_F(SettingsFile, AValueThatIsNotANumberIsRefused)
{
    //THE test of this file. Not zero, not a default: refused, by name.
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes = plenty\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, AnEmptyValueIsRefused)
{
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes =\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, ANumberWithTrailingRubbishIsRefused)
{
    //strtod would happily read the 12 and stop. The whole field has to be a
    //number, or it is not one.
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes = 12 nodes\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, AValueOutOfRangeIsRefused)
{
    //A grid needs at least two points per axis, so four cells.
    EXPECT_THROW(qftbx::readSettings(written("[limits]\n"
                                             "max-grid-cells = 1\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, AFractionWhereAWholeNumberBelongsIsRefused)
{
    //Refused rather than truncated: "10.5 nodes" is a mistake worth pointing
    //at, and truncating it is how a file quietly means something else.
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes = 10.5\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, ARepeatedKeyIsRefused)
{
    //Last-one-wins is how someone spends an afternoon wondering why their
    //edit does nothing.
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes = 10\n"
                                             "max-live-nodes = 20\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, AMalformedLineIsRefused)
{
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes 10\n")),
                 qftbx::InvalidInput);

    EXPECT_THROW(qftbx::readSettings(written("[search\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, AnUnknownKeyIsReportedAndNotFatal)
{
    //A file written by a later version has to be able to start this one, so
    //an unknown key is collected and named rather than refused.
    const qftbx::Settings settings = qftbx::readSettings(written(
        "[search]\n"
        "max-live-nodes = 10\n"
        "wormholes = 3\n"));

    EXPECT_EQ(settings.search.maxLiveNodes, 10u);
    ASSERT_EQ(settings.unknownKeys.size(), 1u);
    EXPECT_EQ(settings.unknownKeys.front(), "search.wormholes");
}

TEST_F(SettingsFile, TheSameKeyInTwoSectionsIsTwoSettings)
{
    //Which is the point of the sections: the key is the whole path.
    const qftbx::Settings settings = qftbx::readSettings(written(
        "[limits]\n"
        "max-grid-cells = 400\n"
        "[search]\n"
        "max-grid-cells = 400\n"));

    EXPECT_EQ(settings.limits.maxGridCells, 400);
    ASSERT_EQ(settings.unknownKeys.size(), 1u)
        << "search.max-grid-cells is not a setting";
    EXPECT_EQ(settings.unknownKeys.front(), "search.max-grid-cells");
}

TEST_F(SettingsFile, AFileThatIsNotThereIsAFileError)
{
    EXPECT_THROW(qftbx::readSettings("/nonexistent/qftbx.conf"),
                 qftbx::FileError);
}

TEST(Settings, LoadingWithNoFileAnywhereGivesTheDefaults)
{
    //loadSettings() searches, and finding nothing is a success: adding this
    //system must not be able to break an installation that never had a file.
    //QFTBX_CONFIG is cleared so the test does not read the developer's own.
    ::unsetenv("QFTBX_CONFIG");

    const qftbx::Settings settings = qftbx::loadSettings();

    //Either it found ./qftbx.conf next to the test runner - it should not,
    //nothing installs one - or it returned the defaults untouched.
    if (settings.source.empty()) {
        EXPECT_EQ(settings.search.maxLiveNodes, 32000000u);
    }
}

TEST(Settings, AFileNamedInTheEnvironmentIsUsed)
{
    const std::string path =
        std::string(QFTBX_TEST_DATA_DIR "/../settings_from_environment.conf");

    std::ofstream file(path);
    file << "[search]\nmax-live-nodes = 77\n";
    file.close();

    ::setenv("QFTBX_CONFIG", path.c_str(), 1);
    const qftbx::Settings settings = qftbx::loadSettings();
    ::unsetenv("QFTBX_CONFIG");
    std::remove(path.c_str());

    EXPECT_EQ(settings.search.maxLiveNodes, 77u);
    EXPECT_EQ(settings.source, path);
}

TEST(Settings, AFileNamedInTheEnvironmentThatCannotBeReadIsAnError)
{
    //Naming a file says it is meant to be used, so falling back silently to
    //the defaults would hide a typo in the variable.
    ::setenv("QFTBX_CONFIG", "/nonexistent/qftbx.conf", 1);

    EXPECT_THROW(qftbx::loadSettings(), qftbx::FileError);

    ::unsetenv("QFTBX_CONFIG");
}

TEST(Settings, TheExampleFileIsValidAndStatesTheRealDefaults)
{
    // qftbx.conf.example is the only documentation anybody reads, so it has
    // to be true. Every setting in it is commented out, and uncommenting all
    // of them has to produce exactly the compiled defaults - which means this
    // test fails the day a default changes in the code and the example is not
    // followed, which is the day it would start lying.
    std::ifstream example(QFTBX_EXAMPLE_CONFIG);
    ASSERT_TRUE(example.good()) << "the example settings file is missing";

    const std::string path =
        std::string(QFTBX_TEST_DATA_DIR "/../settings_example_uncommented.conf");

    std::ofstream uncommented(path);
    std::string line;
    int settingsFound = 0;

    while (std::getline(example, line)) {
        // A commented-out setting looks like "# key = value"; a prose comment
        // does not have an "=" right after one word.
        const std::string body = line.rfind("# ", 0) == 0 ? line.substr(2) : line;
        const std::size_t equals = body.find(" = ");

        if (line.rfind("# ", 0) == 0 && equals != std::string::npos &&
                body.find(' ') == equals) {
            uncommented << body << "\n";
            settingsFound++;
        } else if (line.rfind("[", 0) == 0) {
            uncommented << line << "\n";
        }
    }
    uncommented.close();

    EXPECT_GT(settingsFound, 0) << "no settings found in the example";

    const qftbx::Settings fromExample = qftbx::readSettings(path);
    std::remove(path.c_str());

    const qftbx::Settings defaults;

    EXPECT_EQ(fromExample.limits.maxGridCells, defaults.limits.maxGridCells);
    EXPECT_EQ(fromExample.limits.maxTemplatePoints, defaults.limits.maxTemplatePoints);
    EXPECT_EQ(fromExample.limits.maxFrequencyCount, defaults.limits.maxFrequencyCount);
    EXPECT_EQ(fromExample.limits.maxMagnitude, defaults.limits.maxMagnitude);
    EXPECT_EQ(fromExample.search.maxLiveNodes, defaults.search.maxLiveNodes);

    EXPECT_TRUE(fromExample.unknownKeys.empty())
        << "the example names a setting this build does not know: "
        << (fromExample.unknownKeys.empty() ? std::string() : fromExample.unknownKeys.front());
}

TEST(Settings, TheSearchBudgetReachesTheAlgorithms)
{
    // The setting has to arrive where it matters, and the chain is long -
    // ProjectController to the stage to LoopShaping to whichever algorithm it
    // builds - so this pins that it is connected rather than merely declared.
    // A budget of one node cannot hold a search, so the search refuses.
    qftbx::Settings settings;
    settings.search.maxLiveNodes = 1;

    ProjectController controller;
    controller.applySettings(settings);

    // Enough project to reach the search: the same fixture the stage tests
    // use, built here to keep this file standing on its own.
    std::vector<Parameter> numerator{Parameter(1.0)};
    std::vector<Parameter> denominator{
        Parameter(std::string("a"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(1.0)};

    controller.setPlant(std::make_unique<PolynomialForm>(
        std::string("P"), numerator, denominator,
        Parameter(std::string("kv"), qftbx::Range(1.0, 2.0), 1.5),
        Parameter(0.0)));

    qftbx::SpecificationRecords records;
    qftbx::SpecificationRecord & stability =
            records.at(static_cast<std::size_t>(qftbx::SpecificationType::Stability));
    stability.name = qftbx::specificationName(qftbx::SpecificationType::Stability);
    stability.used = true;
    stability.constant = true;
    stability.height = 5.0;
    stability.omegaStart = 0.1;
    stability.omegaEnd = 10.0;
    controller.setSpecifications(std::move(records));

    controller.setOmega(std::make_unique<Omega>(
        0.1, 10.0, 3, tools::logspace(-1.0, 1.0, 3), Omega::LogSpace));

    qftbx::ParameterGrids grids;
    grids[std::string("a")] = qftbx::math::linspace(1.0, 2.0, 3);
    grids[std::string("kv")] = qftbx::math::linspace(1.0, 2.0, 3);
    ASSERT_TRUE(controller.computeTemplates(std::vector<double>(3, 10.0),
                                            std::move(grids), false));
    ASSERT_TRUE(controller.computeBoundaries(qftbx::Range(-360.0, 0.0), 37,
                                             qftbx::Range(-40.0, 40.0), 21,
                                             -1.0, false, false));

    std::vector<Parameter> zero{Parameter(std::string("z"), qftbx::Range(0.1, 10.0), 1.0)};
    std::vector<Parameter> pole{Parameter(std::string("p"), qftbx::Range(0.1, 10.0), 1.0)};
    controller.setControllerStructure(std::make_unique<ZeroPoleGain>(
        std::string("K"), zero, pole,
        Parameter(std::string("kc"), qftbx::Range(0.01, 100.0), 1.0),
        Parameter(0.0)));

    // With room for one node the list refuses to grow, and the search says so
    // instead of running out of memory.
    EXPECT_THROW(controller.computeLoopShaping(0.5, tools::nt,
                                               qftbx::Range(1e-3, 100.0), 100),
                 qftbx::Exception);
}
