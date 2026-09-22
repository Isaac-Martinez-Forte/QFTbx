/**
 * @file
 * @brief The values a user may change without recompiling.
 *
 * Plain fields with the compiled defaults, grouped the way the file is, and
 * not a map looked up by name: some are read once per node of an interval
 * search, where a string lookup would cost a hundred times the value. Whoever
 * needs one copies it when constructed, so the hot path reads a member. The
 * settings are loaded once at startup and immutable afterwards, which is
 * what makes them safe next to OpenMP and the search's worker thread. A
 * setting is a value with a defensible range; the mathematical and
 * structural constants of the method are not here, since writing them down
 * would configure nothing and break the program.
 *
 * The groups: the limits, which only refuse input and so change no result;
 * what the interval search may spend; the resolution of the nominal stability check, which trades
 * time against how often the check can decide and never touches the
 * criterion; the figures that come from the published algorithms, the group
 * to be careful with because a value changed there changes what the program
 * computes; the interface; the defaults of the dialogs; and the record. Each
 * key, its default and its range is described in docs/CONFIGURATION.md. No
 * test reads the settings file: every one builds its own, so a value here
 * can never change what a test means.
 */

#ifndef QFTBX_SETTINGS_H
#define QFTBX_SETTINGS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace qftbx {

struct Settings {

    struct Limits {
        std::int64_t maxGridCells = 10000000;

        double maxTemplatePoints = 1.0e6;

        std::int32_t maxFrequencyCount = 1000000;

        double maxMagnitude = 1.0e12;
    } limits;

    struct Search {
        std::size_t maxLiveNodes = 32000000;
    } search;

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

        struct McStrategies {
            bool infeasibleMagnitude = true;
            bool infeasiblePhase = true;
            bool feasibleMagnitude = true;
            bool feasiblePhase = true;
            bool bestGain = true;
            bool treeBisection = true;
            bool stages = true;
        } mc;

        bool conservativeBoundaryColumns = true;

        std::int32_t localSearchBudget = 400;

        double gainTolerance = 1.01;

        double certifiedGainTolerance = 1.01;
    } algorithms;

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

    struct Interface {
        std::string language = "system";
        std::string canvas;

        std::string window;

        std::string theme = "system";

        std::int32_t digits = 4;
    } interface;

    struct Log {
        bool enabled = false;

        std::string path;

        std::int32_t sizeLimitKilobytes = 1024;
    } log;

    std::string source;

    std::vector<std::string> unknownKeys;
};

Settings readSettings(const std::string & path);

std::vector<std::string> settingKeys();

std::string userSettingsPath();

void writeSetting(const std::string & path, const std::string & key, const std::string & value);

Settings loadSettings();

void openRecord(const Settings & settings);

}

#endif
