/**
 * @file
 * @brief Tests of the settings file: defaults, sections and what it refuses.
 *
 * The compiled defaults are the contract, since the program runs with no file
 * at all, and a file says only what to change. A value that does not parse
 * never becomes zero or a default: a word, an empty field, trailing text, a
 * fraction where an integer belongs, a value out of range, a repeated key and
 * a malformed line are refused by name, while an unknown key is collected and
 * reported so that a newer file still starts this build. The theme is one
 * of three words and is refused as any other value. The research switches
 * take their own words: the reading of MC2, columns or exact, whose names go
 * both ways; the guide of the exact reading; the reading of the columns; and
 * which reading of the columns is in force follows the reading of MC2.
 * Writing one setting leaves the rest of the file alone. Loading looks in an
 * environment of its own, with its home and its working directory in a
 * temporary folder: a file named in the environment must exist, the working
 * directory comes before the home and the environment before both, none
 * anywhere gives the defaults, and the user file and qftbx-research.conf
 * each take only their own keys, while a file read on its own takes both.
 * The example configuration must uncomment to exactly the compiled defaults
 * and name every user setting the build knows, the three free texts aside,
 * which it shows empty, and a search budget set in the settings must reach
 * the algorithms. Every file a test writes goes to a temporary folder.
 */

#include "src/core/loopshaping/loop_shaping_types.h"
#include <gtest/gtest.h>

#include <QTemporaryDir>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include "src/core/common/exception.h"
#include "src/core/frequencies/omega.h"
#include "src/core/math/sequence_vectors.h"
#include "src/core/math/sequences.h"
#include "src/app/project_controller.h"
#include "src/core/math/range.h"
#include "src/core/specifications/specification_record.h"
#include "src/core/system/parameter.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include <vector>
#include "src/core/project/settings.h"

using namespace qftbx;

namespace {

std::string writtenAt(const std::string & path, const std::string & content)
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream file(path);
    file << content;
    return path;
}

class SettingsFile : public ::testing::Test
{
protected:
    std::string written(const std::string & content)
    {
        return writtenAt(m_folder.filePath("settings.conf").toStdString(), content);
    }

    std::string folder() const { return m_folder.path().toStdString(); }

private:
    QTemporaryDir m_folder;
};

class IsolatedEnvironment
{
public:
    IsolatedEnvironment()
        : m_home(saved("HOME")),
          m_config(saved("QFTBX_CONFIG")),
          m_researchConfig(saved("QFTBX_RESEARCH_CONFIG")),
          m_workingDirectory(std::filesystem::current_path())
    {
        ::setenv("HOME", home().c_str(), 1);
        ::unsetenv("QFTBX_CONFIG");
        ::unsetenv("QFTBX_RESEARCH_CONFIG");
        std::filesystem::create_directories(workingDirectory());
        std::filesystem::current_path(workingDirectory());
    }

    ~IsolatedEnvironment()
    {
        std::filesystem::current_path(m_workingDirectory);
        restore("HOME", m_home);
        restore("QFTBX_CONFIG", m_config);
        restore("QFTBX_RESEARCH_CONFIG", m_researchConfig);
    }

    IsolatedEnvironment(const IsolatedEnvironment &) = delete;
    IsolatedEnvironment & operator=(const IsolatedEnvironment &) = delete;

    std::string home() const { return m_folder.filePath("home").toStdString(); }

    std::string workingDirectory() const { return m_folder.filePath("work").toStdString(); }

    std::string inHome(const std::string & file) const { return home() + "/.config/qftbx/" + file; }

    std::string elsewhere(const std::string & file) const { return m_folder.filePath(QString::fromStdString(file)).toStdString(); }

private:
    static std::optional<std::string> saved(const char * variable)
    {
        const char * value = std::getenv(variable);
        return value != nullptr ? std::optional<std::string>(value) : std::nullopt;
    }

    static void restore(const char * variable, const std::optional<std::string> & value)
    {
        if (value.has_value()) {
            ::setenv(variable, value->c_str(), 1);
        } else {
            ::unsetenv(variable);
        }
    }

    QTemporaryDir m_folder;
    std::optional<std::string> m_home;
    std::optional<std::string> m_config;
    std::optional<std::string> m_researchConfig;
    std::filesystem::path m_workingDirectory;
};

}

TEST_F(SettingsFile, TheDefaultsStandOnTheirOwn)
{
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

    EXPECT_EQ(settings.limits.maxMagnitude, 1.0e12);

    EXPECT_FALSE(settings.source.empty()) << "it has to say which file it read";
}

TEST_F(SettingsFile, AValueThatIsNotANumberIsRefused)
{
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
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes = 12 nodes\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, AValueOutOfRangeIsRefused)
{
    EXPECT_THROW(qftbx::readSettings(written("[limits]\n"
                                             "max-grid-cells = 1\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, TheLanguageIsATextWithTheShapeOfACode)
{
    EXPECT_EQ(qftbx::readSettings(written("[interface]\nlanguage = es\n")).interface.language, "es");
    EXPECT_EQ(qftbx::readSettings(written("[interface]\nlanguage = pt_BR\n")).interface.language, "pt_BR");
    EXPECT_EQ(qftbx::readSettings(written("[interface]\nlanguage = system\n")).interface.language, "system");
    EXPECT_THROW(qftbx::readSettings(written("[interface]\nlanguage = Spanish\n")), qftbx::InvalidInput);
    EXPECT_THROW(qftbx::readSettings(written("[interface]\nlanguage = 3\n")), qftbx::InvalidInput);
}

TEST_F(SettingsFile, TheThemeIsSystemLightOrDark)
{
    EXPECT_EQ(qftbx::Settings().interface.theme, "system");
    for (const std::string theme : {"system", "light", "dark"}) {
        EXPECT_EQ(qftbx::readSettings(written("[interface]\ntheme = " + theme + "\n")).interface.theme, theme);
    }
    EXPECT_THROW(qftbx::readSettings(written("[interface]\ntheme = blue\n")), qftbx::InvalidInput);
    EXPECT_THROW(qftbx::readSettings(written("[interface]\ntheme = Dark\n")), qftbx::InvalidInput);
}

TEST_F(SettingsFile, TheReadingOfMc2IsColumnsOrExact)
{
    using PointReading = qftbx::Settings::Research::PointReading;
    EXPECT_EQ(qftbx::Settings().research.mc2Reading, PointReading::Exact) << "MC2 reads its points exactly";
    EXPECT_EQ(qftbx::readSettings(written("[research]\nmc2-reading = columns\n")).research.mc2Reading,
              PointReading::Columns);
    EXPECT_EQ(qftbx::readSettings(written("[research]\nmc2-reading = exact\n")).research.mc2Reading,
              PointReading::Exact);
    EXPECT_THROW(qftbx::readSettings(written("[research]\nmc2-reading = nearest\n")), qftbx::InvalidInput);
    EXPECT_THROW(qftbx::readSettings(written("[research]\nmc2-reading = 1\n")), qftbx::InvalidInput);

    for (const PointReading reading : {PointReading::Columns, PointReading::Exact}) {
        EXPECT_EQ(qftbx::pointReadingFromName(qftbx::pointReadingName(reading)), reading);
    }
    EXPECT_EQ(qftbx::pointReadingChoices(), "columns or exact");
}

TEST_F(SettingsFile, TheGuideOfTheExactReadingIsNearestOrConservative)
{
    using BoundaryGuide = qftbx::Settings::Research::BoundaryGuide;
    EXPECT_EQ(qftbx::Settings().research.exactGuide, BoundaryGuide::Nearest);
    EXPECT_EQ(qftbx::readSettings(written("[research]\nexact-guide = conservative\n")).research.exactGuide,
              BoundaryGuide::Conservative);
    EXPECT_THROW(qftbx::readSettings(written("[research]\nexact-guide = published\n")), qftbx::InvalidInput);
}

TEST_F(SettingsFile, TheColumnsAreReadConservativelyOrByTheNearestNode)
{
    EXPECT_TRUE(qftbx::Settings().research.conservativeColumns);
    EXPECT_FALSE(qftbx::readSettings(written("[research]\ncolumns = nearest\n")).research.conservativeColumns);
    EXPECT_THROW(qftbx::readSettings(written("[research]\ncolumns = 0\n")), qftbx::InvalidInput);
}

TEST(Settings, TheReadingOfTheColumnsInForceFollowsTheReadingOfMc2)
{
    using PointReading = qftbx::Settings::Research::PointReading;
    using BoundaryGuide = qftbx::Settings::Research::BoundaryGuide;
    qftbx::Settings inForce;
    EXPECT_FALSE(inForce.research.conservativeColumnsInForce()) << "exact points are guided by the nearest node";
    inForce.research.exactGuide = BoundaryGuide::Conservative;
    EXPECT_TRUE(inForce.research.conservativeColumnsInForce());
    inForce.research.mc2Reading = PointReading::Columns;
    inForce.research.conservativeColumns = false;
    EXPECT_FALSE(inForce.research.conservativeColumnsInForce()) << "the columns reading follows research.columns";
}

TEST_F(SettingsFile, WritingASettingLeavesTheRestOfTheFileAlone)
{
    const std::string path = written("# my settings\n"
                                     "[limits]\n"
                                     "max-grid-cells = 500 ; a comment\n"
                                     "\n"
                                     "[interface]\n"
                                     "# the language\n"
                                     "language = en\n"
                                     "\n"
                                     "[stability]\n"
                                     "decades-beyond = 2\n");
    qftbx::writeSetting(path, "interface.language", "es");
    qftbx::writeSetting(path, "limits.max-magnitude", "1e9");
    qftbx::writeSetting(path, "search.max-live-nodes", "1000");

    std::ifstream in(path);
    std::stringstream content;
    content << in.rdbuf();
    EXPECT_EQ(content.str(),
              "# my settings\n"
              "[limits]\n"
              "max-grid-cells = 500 ; a comment\n"
              "max-magnitude = 1e9\n"
              "\n"
              "[interface]\n"
              "# the language\n"
              "language = es\n"
              "\n"
              "[stability]\n"
              "decades-beyond = 2\n"
              "\n"
              "[search]\n"
              "max-live-nodes = 1000\n");

    const qftbx::Settings back = qftbx::readSettings(path);
    EXPECT_EQ(back.interface.language, "es");
    EXPECT_DOUBLE_EQ(back.limits.maxMagnitude, 1e9);
    EXPECT_EQ(back.search.maxLiveNodes, 1000u);
}

TEST_F(SettingsFile, WritingASettingCreatesTheFileAndItsDirectory)
{
    const std::string path = folder() + "/written/deeper/qftbx.conf";
    qftbx::writeSetting(path, "interface.language", "es");
    EXPECT_EQ(qftbx::readSettings(path).interface.language, "es");
}

TEST_F(SettingsFile, AFractionWhereAWholeNumberBelongsIsRefused)
{
    EXPECT_THROW(qftbx::readSettings(written("[search]\n"
                                             "max-live-nodes = 10.5\n")),
                 qftbx::InvalidInput);
}

TEST_F(SettingsFile, ARepeatedKeyIsRefused)
{
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
    const IsolatedEnvironment environment;

    const qftbx::Settings settings = qftbx::loadSettings();

    EXPECT_TRUE(settings.source.empty());
    EXPECT_TRUE(settings.researchSource.empty());
    EXPECT_EQ(settings.search.maxLiveNodes, qftbx::Settings().search.maxLiveNodes);
    EXPECT_EQ(settings.research.mc2Reading, qftbx::Settings().research.mc2Reading);
    EXPECT_TRUE(settings.unknownKeys.empty());
}

TEST(Settings, AFileNamedInTheEnvironmentIsUsed)
{
    const IsolatedEnvironment environment;
    const std::string path = writtenAt(environment.elsewhere("from-environment.conf"), "[search]\nmax-live-nodes = 77\n");

    ::setenv("QFTBX_CONFIG", path.c_str(), 1);
    const qftbx::Settings settings = qftbx::loadSettings();

    EXPECT_EQ(settings.search.maxLiveNodes, 77u);
    EXPECT_EQ(settings.source, path);
}

TEST(Settings, TheWorkingDirectoryComesBeforeTheHomeAndTheEnvironmentBeforeBoth)
{
    const IsolatedEnvironment environment;
    const std::string home = writtenAt(environment.inHome("qftbx.conf"), "[search]\nmax-live-nodes = 22\n");
    const std::string researchHome = writtenAt(environment.inHome("qftbx-research.conf"), "[research]\nfamily-gate = 0\n");

    qftbx::Settings settings = qftbx::loadSettings();
    EXPECT_EQ(settings.source, home);
    EXPECT_EQ(settings.search.maxLiveNodes, 22u);
    EXPECT_EQ(settings.researchSource, researchHome);
    EXPECT_FALSE(settings.research.familyGate);

    writtenAt(environment.workingDirectory() + "/qftbx.conf", "[search]\nmax-live-nodes = 11\n");
    writtenAt(environment.workingDirectory() + "/qftbx-research.conf", "[research]\ncolumns = nearest\n");
    settings = qftbx::loadSettings();
    EXPECT_EQ(settings.source, "qftbx.conf");
    EXPECT_EQ(settings.search.maxLiveNodes, 11u);
    EXPECT_EQ(settings.researchSource, "qftbx-research.conf");
    EXPECT_FALSE(settings.research.conservativeColumns);
    EXPECT_TRUE(settings.research.familyGate) << "only the first file found is read";

    const std::string named = writtenAt(environment.elsewhere("named.conf"), "[search]\nmax-live-nodes = 33\n");
    const std::string researchNamed = writtenAt(environment.elsewhere("named-research.conf"), "[research]\nexact-guide = conservative\n");
    ::setenv("QFTBX_CONFIG", named.c_str(), 1);
    ::setenv("QFTBX_RESEARCH_CONFIG", researchNamed.c_str(), 1);
    settings = qftbx::loadSettings();
    EXPECT_EQ(settings.source, named);
    EXPECT_EQ(settings.search.maxLiveNodes, 33u);
    EXPECT_EQ(settings.researchSource, researchNamed);
    EXPECT_EQ(settings.research.exactGuide, qftbx::Settings::Research::BoundaryGuide::Conservative);
    EXPECT_TRUE(settings.research.conservativeColumns);
}

TEST(Settings, EachFileTakesOnlyItsOwnKeys)
{
    const IsolatedEnvironment environment;
    const std::string user = writtenAt(environment.elsewhere("user-keys.conf"),
                                       "[search]\nmax-live-nodes = 77\n[research]\nfamily-gate = 0\n");
    const std::string research = writtenAt(environment.elsewhere("research-keys.conf"),
                                           "[research]\ncolumns = nearest\n[search]\nmax-live-nodes = 5\n");

    ::setenv("QFTBX_CONFIG", user.c_str(), 1);
    ::setenv("QFTBX_RESEARCH_CONFIG", research.c_str(), 1);
    const qftbx::Settings settings = qftbx::loadSettings();
    const qftbx::Settings alone = qftbx::readSettings(user);

    EXPECT_EQ(settings.source, user);
    EXPECT_EQ(settings.researchSource, research);
    EXPECT_EQ(settings.search.maxLiveNodes, 77u) << "the research file does not set a user key";
    EXPECT_TRUE(settings.research.familyGate) << "the user file does not set a research key";
    EXPECT_FALSE(settings.research.conservativeColumns);
    ASSERT_EQ(settings.unknownKeys.size(), 2u);
    EXPECT_EQ(settings.unknownKeys[0], "research.family-gate");
    EXPECT_EQ(settings.unknownKeys[1], "search.max-live-nodes");

    EXPECT_TRUE(alone.unknownKeys.empty()) << "a file read on its own takes both";
    EXPECT_FALSE(alone.research.familyGate);
    EXPECT_EQ(alone.search.maxLiveNodes, 77u);
}

TEST(Settings, AFileNamedInTheEnvironmentThatCannotBeReadIsAnError)
{
    const IsolatedEnvironment environment;
    ::setenv("QFTBX_CONFIG", environment.elsewhere("missing.conf").c_str(), 1);

    EXPECT_THROW(qftbx::loadSettings(), qftbx::FileError);
}

TEST(Settings, TheExampleFileIsValidAndStatesTheRealDefaults)
{
    std::ifstream example(QFTBX_EXAMPLE_CONFIG);
    ASSERT_TRUE(example.good()) << "the example settings file is missing";

    QTemporaryDir folder;
    const std::string path = folder.filePath("example-uncommented.conf").toStdString();

    std::ofstream uncommented(path);
    std::string line;
    int settingsFound = 0;

    while (std::getline(example, line)) {
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

    const qftbx::Settings defaults;

    EXPECT_EQ(fromExample.limits.maxGridCells, defaults.limits.maxGridCells);
    EXPECT_EQ(fromExample.limits.maxTemplatePoints, defaults.limits.maxTemplatePoints);
    EXPECT_EQ(fromExample.limits.maxFrequencyCount, defaults.limits.maxFrequencyCount);
    EXPECT_EQ(fromExample.limits.maxMagnitude, defaults.limits.maxMagnitude);
    EXPECT_EQ(fromExample.search.maxLiveNodes, defaults.search.maxLiveNodes);

    EXPECT_EQ(fromExample.defaults.phaseStart, defaults.defaults.phaseStart);
    EXPECT_EQ(fromExample.defaults.phaseEnd, defaults.defaults.phaseEnd);
    EXPECT_EQ(fromExample.defaults.phasePoints, defaults.defaults.phasePoints);
    EXPECT_EQ(fromExample.defaults.magnitudeStart, defaults.defaults.magnitudeStart);
    EXPECT_EQ(fromExample.defaults.magnitudeEnd, defaults.defaults.magnitudeEnd);
    EXPECT_EQ(fromExample.defaults.magnitudePoints, defaults.defaults.magnitudePoints);
    EXPECT_EQ(fromExample.defaults.templatePointCount, defaults.defaults.templatePointCount);
    EXPECT_EQ(fromExample.defaults.epsilonInNichols, defaults.defaults.epsilonInNichols);
    EXPECT_EQ(fromExample.defaults.dbPerDegree, defaults.defaults.dbPerDegree);
    EXPECT_EQ(fromExample.defaults.loopStart, defaults.defaults.loopStart);
    EXPECT_EQ(fromExample.defaults.loopEnd, defaults.defaults.loopEnd);
    EXPECT_EQ(fromExample.defaults.loopPointCount, defaults.defaults.loopPointCount);

    EXPECT_EQ(fromExample.stability.baseGridPoints, defaults.stability.baseGridPoints);
    EXPECT_EQ(fromExample.stability.decadesBeyond, defaults.stability.decadesBeyond);
    EXPECT_EQ(fromExample.stability.maxPhaseStepDegrees, defaults.stability.maxPhaseStepDegrees);
    EXPECT_EQ(fromExample.stability.refinementBudget, defaults.stability.refinementBudget);

    EXPECT_EQ(fromExample.interface.language, defaults.interface.language);
    EXPECT_EQ(fromExample.algorithms.templateRepresentatives,
              defaults.algorithms.templateRepresentatives);
    EXPECT_EQ(fromExample.algorithms.maxNarrowingPasses,
              defaults.algorithms.maxNarrowingPasses);
    EXPECT_EQ(fromExample.algorithms.mrNicholsEpsilon, defaults.algorithms.mrNicholsEpsilon);
    EXPECT_EQ(fromExample.algorithms.wholeTemplateIfNoContour,
              defaults.algorithms.wholeTemplateIfNoContour);
    EXPECT_EQ(fromExample.algorithms.alphaShapeContour,
              defaults.algorithms.alphaShapeContour);
    EXPECT_EQ(fromExample.algorithms.borderSweep, defaults.algorithms.borderSweep);
    EXPECT_EQ(fromExample.algorithms.closedFormColumns, defaults.algorithms.closedFormColumns);
    EXPECT_EQ(fromExample.algorithms.localSearchBudget,
              defaults.algorithms.localSearchBudget);
    EXPECT_EQ(fromExample.algorithms.gainTolerance, defaults.algorithms.gainTolerance);
    EXPECT_EQ(fromExample.algorithms.certifiedGainTolerance,
              defaults.algorithms.certifiedGainTolerance);
    EXPECT_EQ(fromExample.defaults.boundariesFromCloud, defaults.defaults.boundariesFromCloud);
    EXPECT_EQ(fromExample.interface.digits, defaults.interface.digits);
    EXPECT_EQ(fromExample.log.enabled, defaults.log.enabled);
    EXPECT_EQ(fromExample.log.sizeLimitKilobytes, defaults.log.sizeLimitKilobytes);

    int userKeys = 0;
    for (const std::string & key : qftbx::settingKeys()) {
        const bool freeText = key == "interface.canvas" || key == "interface.window" || key == "log.path";
        userKeys += key.rfind("research.", 0) == 0 || freeText ? 0 : 1;
    }
    EXPECT_EQ(settingsFound, userKeys)
        << "a setting was added to the code and not to qftbx.conf.example";

    EXPECT_TRUE(fromExample.unknownKeys.empty())
        << "the example names a setting this build does not know: "
        << (fromExample.unknownKeys.empty() ? std::string() : fromExample.unknownKeys.front());
}

TEST(Settings, TheSearchBudgetReachesTheAlgorithms)
{
    qftbx::Settings settings;
    settings.search.maxLiveNodes = 1;

    ProjectController controller;
    controller.applySettings(settings);

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
        0.1, 10.0, 3, qftbx::logspace(-1.0, 1.0, 3), Omega::LogSpace));

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

    EXPECT_THROW(controller.computeLoopShaping(0.5, qftbx::nt,
                                               qftbx::Range(1e-3, 100.0), 100),
                 qftbx::Exception);
}
