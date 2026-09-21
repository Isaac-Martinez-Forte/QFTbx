/**
 * @file
 * @brief Tests of the settings file: defaults, sections and what it refuses.
 *
 * The compiled defaults are the contract, since the program runs with no file
 * at all, and a file says only what to change. A value that does not parse
 * never becomes zero or a default: a word, an empty field, trailing text, a
 * fraction where an integer belongs, a value out of range, a repeated key and
 * a malformed line are refused by name, while an unknown key is collected and
 * reported so that a newer file still starts this build. A file named in the
 * environment must exist; none anywhere gives the defaults. Writing one
 * setting leaves the rest of the file alone. The example configuration must
 * uncomment to exactly the compiled defaults and name every setting the build
 * knows, and a search budget set in the settings must reach the algorithms.
 */

#include "src/core/loopshaping/loop_shaping_types.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
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

class SettingsFile : public ::testing::Test
{
protected:
    void TearDown() override
    {
        if (!m_path.empty()) {
            std::remove(m_path.c_str());
        }
    }

    std::string written(const std::string & content)
    {
        m_path = std::string(QFTBX_TEST_DATA_DIR "/../settings_under_test_")
                 + ::testing::UnitTest::GetInstance()->current_test_info()->name()
                 + ".conf";

        std::ofstream file(m_path);
        file << content;
        file.close();

        return m_path;
    }

private:
    std::string m_path;
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
    const std::string directory = std::string(QFTBX_TEST_DATA_DIR "/../settings_written_dir");
    const std::string path = directory + "/deeper/qftbx.conf";
    std::filesystem::remove_all(directory);
    qftbx::writeSetting(path, "interface.language", "es");
    EXPECT_EQ(qftbx::readSettings(path).interface.language, "es");
    std::filesystem::remove_all(directory);
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
    ::unsetenv("QFTBX_CONFIG");

    const qftbx::Settings settings = qftbx::loadSettings();

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
    ::setenv("QFTBX_CONFIG", "/nonexistent/qftbx.conf", 1);

    EXPECT_THROW(qftbx::loadSettings(), qftbx::FileError);

    ::unsetenv("QFTBX_CONFIG");
}

TEST(Settings, TheExampleFileIsValidAndStatesTheRealDefaults)
{
    std::ifstream example(QFTBX_EXAMPLE_CONFIG);
    ASSERT_TRUE(example.good()) << "the example settings file is missing";

    const std::string path =
        std::string(QFTBX_TEST_DATA_DIR "/../settings_example_uncommented.conf");

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
    std::remove(path.c_str());

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
    EXPECT_EQ(fromExample.algorithms.conservativeBoundaryColumns,
              defaults.algorithms.conservativeBoundaryColumns);
    EXPECT_EQ(fromExample.algorithms.wholeTemplateIfNoContour,
              defaults.algorithms.wholeTemplateIfNoContour);
    EXPECT_EQ(fromExample.algorithms.alphaShapeContour,
              defaults.algorithms.alphaShapeContour);
    EXPECT_EQ(fromExample.algorithms.borderSweep, defaults.algorithms.borderSweep);
    EXPECT_EQ(fromExample.algorithms.closedFormColumns, defaults.algorithms.closedFormColumns);
    EXPECT_EQ(fromExample.algorithms.mc.infeasibleMagnitude, defaults.algorithms.mc.infeasibleMagnitude);
    EXPECT_EQ(fromExample.algorithms.mc.infeasiblePhase, defaults.algorithms.mc.infeasiblePhase);
    EXPECT_EQ(fromExample.algorithms.mc.feasibleMagnitude, defaults.algorithms.mc.feasibleMagnitude);
    EXPECT_EQ(fromExample.algorithms.mc.feasiblePhase, defaults.algorithms.mc.feasiblePhase);
    EXPECT_EQ(fromExample.algorithms.mc.bestGain, defaults.algorithms.mc.bestGain);
    EXPECT_EQ(fromExample.algorithms.mc.treeBisection, defaults.algorithms.mc.treeBisection);
    EXPECT_EQ(fromExample.algorithms.mc.stages, defaults.algorithms.mc.stages);
    EXPECT_EQ(fromExample.algorithms.localSearchBudget,
              defaults.algorithms.localSearchBudget);
    EXPECT_EQ(fromExample.algorithms.gainTolerance, defaults.algorithms.gainTolerance);
    EXPECT_EQ(fromExample.algorithms.certifiedGainTolerance,
              defaults.algorithms.certifiedGainTolerance);
    EXPECT_EQ(fromExample.defaults.boundariesFromCloud, defaults.defaults.boundariesFromCloud);
    EXPECT_EQ(fromExample.interface.digits, defaults.interface.digits);
    EXPECT_EQ(fromExample.log.enabled, defaults.log.enabled);
    EXPECT_EQ(fromExample.log.sizeLimitKilobytes, defaults.log.sizeLimitKilobytes);

    EXPECT_EQ(settingsFound, 45)
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
