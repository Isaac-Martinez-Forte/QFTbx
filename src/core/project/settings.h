/**
 * @file
 * @brief The values a user may change without recompiling.
 *
 * Plain fields with the compiled defaults, not a map looked up by name:
 * some are read once per node of a search, so whoever needs one copies it
 * when constructed. The settings are loaded once at startup and immutable
 * afterwards. The constants of the method are not settings.
 *
 * The groups, in the same order here, in the table that reads them, in the
 * example file and in the guide: the interface; the record; the limits,
 * which only refuse input; what the interval search may spend; the
 * defaults of the dialogs; the resolution of the nominal stability check;
 * and the figures that come from the published algorithms, which change
 * what the program computes. docs/CONFIGURATION.md describes each key. The
 * tests build their own settings and never read a file.
 *
 * Research holds what is not for a user: the variants that reproduce the
 * published algorithms and the switches the measurements turn, read from a
 * file of their own, qftbx-research.conf, found as qftbx.conf is
 * (QFTBX_RESEARCH_CONFIG, the working directory, ~/.config/qftbx). Each
 * file takes only its own keys, the benchmark and the command-line solver
 * read both, and the defaults are the program's own way of working: MC2
 * reads the points exactly, contracts the gain of every box and goes
 * without the feasible magnitude cut.
 */

#ifndef QFTBX_SETTINGS_H
#define QFTBX_SETTINGS_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qftbx {

struct Settings {

    struct Interface {
        std::string language = "system";

        std::string theme = "system";

        std::int32_t digits = 4;

        std::string canvas;

        std::string window;
    } interface;

    struct Log {
        bool enabled = false;

        std::string path;

        std::int32_t sizeLimitKilobytes = 1024;
    } log;

    struct Limits {
        std::int64_t maxGridCells = 10000000;

        double maxTemplatePoints = 1.0e6;

        std::int32_t maxFrequencyCount = 1000000;

        double maxMagnitude = 1.0e12;
    } limits;

    struct Search {
        std::size_t maxLiveNodes = 32000000;
    } search;

    struct Defaults {
        double phaseStart = -360.0;
        double phaseEnd = 0.0;
        std::int32_t phasePoints = 361;
        double magnitudeStart = -60.0;
        double magnitudeEnd = 60.0;
        std::int32_t magnitudePoints = 121;

        bool boundariesFromCloud = false;

        std::int32_t templatePointCount = 25;

        bool epsilonInNichols = true;
        double dbPerDegree = 1.0;

        double loopStart = 1.0e-9;
        double loopEnd = 10.0;
        std::int32_t loopPointCount = 100;
    } defaults;

    struct Stability {
        std::int32_t baseGridPoints = 3000;

        double decadesBeyond = 3.0;

        double maxPhaseStepDegrees = 30.0;

        std::int32_t refinementBudget = 200000;
    } stability;

    struct Algorithms {
        std::int32_t templateRepresentatives = 9;

        std::int32_t maxNarrowingPasses = 8;

        bool mrNicholsEpsilon = false;

        bool wholeTemplateIfNoContour = true;

        bool alphaShapeContour = false;

        bool borderSweep = false;

        bool closedFormColumns = false;

        std::int32_t localSearchBudget = 400;

        double gainTolerance = 1.01;

        double certifiedGainTolerance = 1.01;
    } algorithms;

    struct Research {
        enum class PointReading { Columns, Exact };

        enum class BoundaryGuide { Nearest, Conservative };

        PointReading mc2Reading = PointReading::Exact;

        bool conservativeColumns = true;

        bool familyGate = true;

        BoundaryGuide exactGuide = BoundaryGuide::Nearest;

        struct McStrategies {
            bool infeasibleMagnitude = true;
            bool infeasiblePhase = true;
            bool feasibleMagnitude = true;
            bool feasiblePhase = true;
            bool bestGain = true;
            bool treeBisection = true;
            bool stages = true;
        } mc;

        McStrategies mc2 = [] {
            McStrategies strategies;
            strategies.feasibleMagnitude = false;
            return strategies;
        }();

        bool mc2GainContraction = true;

        bool conservativeColumnsInForce() const
        {
            return mc2Reading == PointReading::Exact ? exactGuide == BoundaryGuide::Conservative
                                                     : conservativeColumns;
        }
    } research;

    std::string source;

    std::string researchSource;

    std::vector<std::string> unknownKeys;
};

const char * pointReadingName(Settings::Research::PointReading reading);

std::optional<Settings::Research::PointReading> pointReadingFromName(const std::string & name);

std::string pointReadingChoices();

Settings readSettings(const std::string & path);

std::vector<std::string> settingKeys();

std::string userSettingsPath();

void writeSetting(const std::string & path, const std::string & key, const std::string & value);

Settings loadSettings();

void openRecord(const Settings & settings);

}

#endif
