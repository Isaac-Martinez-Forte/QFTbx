#include "src/core/settings.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>

#include "src/core/exception.h"

namespace qftbx {

namespace {

std::string trimmed(const std::string & text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::string();
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

//A number, or nothing. strtod and strtoll both report where they stopped, and
//anything left over means the line was not a number - which is the whole
//point: a value that does not parse must NOT become zero.
bool wholeStringIsReal(const std::string & text, double & value)
{
    if (text.empty()) {
        return false;
    }

    const char * begin = text.c_str();
    char * end = nullptr;
    const double parsed = std::strtod(begin, &end);

    if (end != begin + text.size() || !std::isfinite(parsed)) {
        return false;
    }

    value = parsed;
    return true;
}

//One entry per setting: the key it answers to, and what to do with the text.
//A table and not a chain of ifs, because the table is also what tells the
//reader whether a key is unknown.
struct Binding {
    const char * key;
    std::function<void (const std::string & text, std::int64_t line,
                        Settings & into)> apply;
};

[[noreturn]] void refuse(const std::string & key, std::int64_t line,
                         const std::string & wanted)
{
    throw InvalidInput("settings, line " + std::to_string(line) + ": \"" + key
                       + "\" needs " + wanted);
}

//Reads a real in [lowest, highest], both included.
double realIn(const std::string & text, const std::string & key,
              std::int64_t line, double lowest, double highest)
{
    double value = 0.0;

    if (!wholeStringIsReal(text, value)) {
        refuse(key, line, "a finite number, and \"" + text + "\" is not one");
    }

    if (value < lowest || value > highest) {
        refuse(key, line, "a value between " + std::to_string(lowest) + " and "
               + std::to_string(highest) + ", not " + text);
    }

    return value;
}

//Reads a whole number in [lowest, highest]. Fractions are refused rather than
//truncated: "10.5 points" is a mistake worth pointing at, not rounding.
double wholeIn(const std::string & text, const std::string & key,
               std::int64_t line, double lowest, double highest)
{
    const double value = realIn(text, key, line, lowest, highest);

    if (value != std::floor(value)) {
        refuse(key, line, "a whole number, not " + text);
    }

    return value;
}

const std::vector<Binding> & bindings()
{
    static const std::vector<Binding> table = {
        //A grid needs at least two points per axis, so four cells; the ceiling
        //is what an std::int64_t can multiply without overflowing.
        {"limits.max-grid-cells",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.limits.maxGridCells = static_cast<std::int64_t>(
                 wholeIn(text, "limits.max-grid-cells", line, 4.0, 1.0e15));
         }},
        {"limits.max-template-points",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.limits.maxTemplatePoints =
                 wholeIn(text, "limits.max-template-points", line, 1.0, 1.0e12);
         }},
        {"limits.max-frequency-count",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.limits.maxFrequencyCount = static_cast<std::int32_t>(
                 wholeIn(text, "limits.max-frequency-count", line, 1.0,
                         static_cast<double>(std::numeric_limits<std::int32_t>::max())));
         }},
        {"limits.max-magnitude",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.limits.maxMagnitude =
                 realIn(text, "limits.max-magnitude", line, 1.0, 1.0e300);
         }},
        //[stability] - the resolution of the nominal stability check. Time
        //against how reliably it decides; the criterion itself is not here.
        {"stability.base-grid-points",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.stability.baseGridPoints = static_cast<std::int32_t>(
                 wholeIn(text, "stability.base-grid-points", line, 10.0, 1.0e7));
         }},
        {"stability.decades-beyond",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.stability.decadesBeyond =
                 realIn(text, "stability.decades-beyond", line, 0.0, 20.0);
         }},
        {"stability.max-phase-step-degrees",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.stability.maxPhaseStepDegrees =
                 realIn(text, "stability.max-phase-step-degrees", line, 0.1, 180.0);
         }},
        {"stability.refinement-budget",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.stability.refinementBudget = static_cast<std::int32_t>(
                 wholeIn(text, "stability.refinement-budget", line, 1.0, 1.0e9));
         }},

        //[algorithms] - figures from the papers. These change WHAT is
        //computed, which is why the header says so next to each one.
        {"algorithms.template-representatives",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.templateRepresentatives = static_cast<std::int32_t>(
                 wholeIn(text, "algorithms.template-representatives", line, 2.0, 1000.0));
         }},
        {"algorithms.max-narrowing-passes",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.maxNarrowingPasses = static_cast<std::int32_t>(
                 wholeIn(text, "algorithms.max-narrowing-passes", line, 1.0, 1000.0));
         }},
        {"algorithms.local-search-budget",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.localSearchBudget = static_cast<std::int32_t>(
                 wholeIn(text, "algorithms.local-search-budget", line, 1.0, 1.0e7));
         }},
        //Strictly above 1: a ratio of exactly 1 is a bisection that never
        //ends, which is a hang and not a tighter answer.
        {"algorithms.gain-tolerance",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.gainTolerance =
                 realIn(text, "algorithms.gain-tolerance", line, 1.0000001, 10.0);
         }},
        {"algorithms.certified-gain-tolerance",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.certifiedGainTolerance =
                 realIn(text, "algorithms.certified-gain-tolerance", line, 1.0000001, 10.0);
         }},

        //[defaults.boundary-grid] - the Nichols grid a boundary computation
        //starts with. Phase in degrees, magnitude in decibels. The ranges are
        //wide because a phase axis has to span at least 360 degrees for the
        //union to close, which the computation checks for itself.
        {"defaults.boundary-grid.phase-start",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.phaseStart =
                 realIn(text, "defaults.boundary-grid.phase-start", line, -3600.0, 3600.0);
         }},
        {"defaults.boundary-grid.phase-end",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.phaseEnd =
                 realIn(text, "defaults.boundary-grid.phase-end", line, -3600.0, 3600.0);
         }},
        {"defaults.boundary-grid.phase-points",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.phasePoints = static_cast<std::int32_t>(
                 wholeIn(text, "defaults.boundary-grid.phase-points", line, 2.0, 1.0e6));
         }},
        {"defaults.boundary-grid.magnitude-start",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.magnitudeStart =
                 realIn(text, "defaults.boundary-grid.magnitude-start", line, -1000.0, 1000.0);
         }},
        {"defaults.boundary-grid.magnitude-end",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.magnitudeEnd =
                 realIn(text, "defaults.boundary-grid.magnitude-end", line, -1000.0, 1000.0);
         }},
        {"defaults.boundary-grid.magnitude-points",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.magnitudePoints = static_cast<std::int32_t>(
                 wholeIn(text, "defaults.boundary-grid.magnitude-points", line, 2.0, 1.0e6));
         }},

        {"defaults.templates.point-count",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.templatePointCount = static_cast<std::int32_t>(
                 wholeIn(text, "defaults.templates.point-count", line, 1.0, 1.0e6));
         }},

        //[defaults.loop-shaping] - in rad/s, like every other frequency.
        {"defaults.loop-shaping.start",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.loopStart =
                 realIn(text, "defaults.loop-shaping.start", line, 1.0e-300, 1.0e300);
         }},
        {"defaults.loop-shaping.end",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.loopEnd =
                 realIn(text, "defaults.loop-shaping.end", line, 1.0e-300, 1.0e300);
         }},
        {"defaults.loop-shaping.point-count",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.loopPointCount = static_cast<std::int32_t>(
                 wholeIn(text, "defaults.loop-shaping.point-count", line, 2.0, 1.0e6));
         }},

        {"search.max-live-nodes",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.search.maxLiveNodes = static_cast<std::size_t>(
                 wholeIn(text, "search.max-live-nodes", line, 1.0, 1.0e15));
         }},
    };

    return table;
}

std::string environmentPath()
{
    const char * named = std::getenv("QFTBX_CONFIG");
    return named != nullptr ? std::string(named) : std::string();
}

std::string homePath()
{
    const char * home = std::getenv("HOME");
    if (home == nullptr) {
        return std::string();
    }

    return std::string(home) + "/.config/qftbx/qftbx.conf";
}

bool readable(const std::string & path)
{
    if (path.empty()) {
        return false;
    }

    std::ifstream file(path);
    return file.good();
}

} // namespace

Settings readSettings(const std::string & path)
{
    std::ifstream file(path);
    if (!file.good()) {
        throw FileError("the settings file cannot be read: " + path);
    }

    Settings settings;
    settings.source = path;

    std::string section;
    std::string line;
    std::int64_t number = 0;
    std::vector<std::string> seen;

    while (std::getline(file, line)) {
        number++;

        //Comments run to the end of the line and can start one. Both markers
        //are accepted because both are what people type.
        const std::size_t comment = line.find_first_of("#;");
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }

        const std::string content = trimmed(line);
        if (content.empty()) {
            continue;
        }

        if (content.front() == '[') {
            if (content.back() != ']') {
                throw InvalidInput("settings, line " + std::to_string(number)
                                   + ": a section needs its closing bracket");
            }

            section = trimmed(content.substr(1, content.size() - 2));
            if (section.empty()) {
                throw InvalidInput("settings, line " + std::to_string(number)
                                   + ": a section needs a name");
            }
            continue;
        }

        const std::size_t equals = content.find('=');
        if (equals == std::string::npos) {
            throw InvalidInput("settings, line " + std::to_string(number)
                               + ": expected \"key = value\", found \""
                               + content + "\"");
        }

        const std::string name = trimmed(content.substr(0, equals));
        const std::string value = trimmed(content.substr(equals + 1));

        if (name.empty()) {
            throw InvalidInput("settings, line " + std::to_string(number)
                               + ": the key is missing");
        }

        //A key outside any section would be ambiguous with a nested one.
        const std::string key = section.empty() ? name : section + "." + name;

        //Twice in one file is a mistake, and a silent last-one-wins is how
        //someone spends an afternoon wondering why their edit does nothing.
        if (std::find(seen.begin(), seen.end(), key) != seen.end()) {
            throw InvalidInput("settings, line " + std::to_string(number)
                               + ": \"" + key + "\" is set more than once");
        }
        seen.push_back(key);

        const auto & table = bindings();
        const auto found = std::find_if(table.begin(), table.end(),
                                        [&key](const Binding & binding) {
                                            return key == binding.key;
                                        });

        if (found == table.end()) {
            //Not an error: a file written by a later version has to be able
            //to start this one.
            settings.unknownKeys.push_back(key);
            continue;
        }

        //The line goes in, so a refused value points at where it is written
        //and not just at the key.
        found->apply(value, number, settings);
    }

    return settings;
}

Settings loadSettings()
{
    //Named explicitly: naming a file says it is meant to be used, so failing
    //to read THAT one is an error rather than a reason to fall through.
    const std::string named = environmentPath();
    if (!named.empty()) {
        return readSettings(named);
    }

    for (const std::string & candidate : {std::string("qftbx.conf"), homePath()}) {
        if (readable(candidate)) {
            return readSettings(candidate);
        }
    }

    //No file: the compiled defaults, and an empty source saying so.
    return Settings();
}

} // namespace qftbx
