/**
 * @file
 * @brief Reading and writing of the settings file.
 *
 * The file is INI-like: sections in brackets, "key = value" lines, and
 * comments from '#' or ';' to the end of the line. A table binds each known
 * key to the field it sets and the range it accepts, and is also what tells
 * an unknown key, which is kept as a warning so a file from a later version
 * still starts. A value is parsed as a whole string and never becomes zero
 * when it is not a number; fractions are refused where a whole number is
 * wanted, duplicates are refused, and a ratio tolerance must be strictly
 * above one so a bisection can end. Writing one key edits the file in place,
 * keeping every other line as the person left it.
 */

#include "src/core/common/record.h"
#include "src/core/project/settings.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>

#include "src/core/common/exception.h"
#include "src/core/common/text_tokens.h"

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

struct Binding {
    const char * key;
    std::function<void (const std::string & text, std::int64_t line,
                        Settings & into)> apply;
};

[[noreturn]] void refuse(const std::string & key, std::int64_t line,
                         const std::string & wanted)
{
    throw InvalidInput(QFTBX_TR("Core", "settings, line %1: \"%2\" needs %3").arg(line).arg(key).arg(wanted));
}

double realIn(const std::string & text, const std::string & key,
              std::int64_t line, double lowest, double highest)
{
    double value = 0.0;

    if (!wholeStringIsReal(text, value)) {
        refuse(key, line, "a finite number, and \"" + text + "\" is not one");
    }

    if (value < lowest || value > highest) {
        refuse(key, line, "a value between " + text::number(lowest) + " and "
               + text::number(highest) + ", not " + text);
    }

    return value;
}

double wholeIn(const std::string & text, const std::string & key,
               std::int64_t line, double lowest, double highest)
{
    const double value = realIn(text, key, line, lowest, highest);

    if (value != std::floor(value)) {
        refuse(key, line, "a whole number, not " + text);
    }

    return value;
}

std::string languageIn(const std::string & text, const std::string & key, std::int64_t line)
{
    if (text == "system") {
        return text;
    }
    const std::size_t underscore = text.find('_');
    const std::string language = text.substr(0, underscore);
    const std::string region = underscore == std::string::npos ? std::string() : text.substr(underscore + 1);
    const auto lower = [](const std::string & s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= 'a' && c <= 'z'; });
    };
    const auto upper = [](const std::string & s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= 'A' && c <= 'Z'; });
    };
    if (!(language.size() >= 2 && language.size() <= 3 && lower(language))
            || (underscore != std::string::npos && !(region.size() == 2 && upper(region)))) {
        refuse(key, line, "\"system\" or a language code such as \"es\" or \"pt_BR\", not " + text);
    }
    return text;
}

const std::vector<Binding> & bindings()
{
    static const std::vector<Binding> table = {
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

        {"interface.language",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.interface.language = languageIn(text, "interface.language", line);
         }},

        {"interface.canvas",
         [](const std::string & text, std::int64_t, Settings & into) {
             into.interface.canvas = text;
         }},

        {"interface.theme",
         [](const std::string & text, std::int64_t line, Settings & into) {
             if (text != "system" && text != "light" && text != "dark") {
                 throw ParseError(QFTBX_TR("Core", "interface.theme must be system, light or dark: '%1'").arg(text),
                                  line);
             }
             into.interface.theme = text;
         }},

        {"interface.digits",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.interface.digits = static_cast<std::int32_t>(
                 wholeIn(text, "interface.digits", line, 1.0, 17.0));
         }},

        {"interface.window",
         [](const std::string & text, std::int64_t, Settings & into) {
             into.interface.window = text;
         }},

        {"log.enabled",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.log.enabled = wholeIn(text, "log.enabled", line, 0.0, 1.0) != 0.0;
         }},
        {"log.path",
         [](const std::string & text, std::int64_t, Settings & into) {
             into.log.path = text;
         }},
        {"log.size-limit-kilobytes",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.log.sizeLimitKilobytes = static_cast<std::int32_t>(
                 wholeIn(text, "log.size-limit-kilobytes", line, 16.0, 1048576.0));
         }},

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
        {"algorithms.mr-nichols-epsilon",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mrNicholsEpsilon =
                 wholeIn(text, "algorithms.mr-nichols-epsilon", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.conservative-boundary-columns",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.conservativeBoundaryColumns =
                 wholeIn(text, "algorithms.conservative-boundary-columns", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.family-stability-gate",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.familyStabilityGate =
                 wholeIn(text, "algorithms.family-stability-gate", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.whole-template-if-no-contour",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.wholeTemplateIfNoContour =
                 wholeIn(text, "algorithms.whole-template-if-no-contour", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.alpha-shape-contour",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.alphaShapeContour =
                 wholeIn(text, "algorithms.alpha-shape-contour", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.border-sweep",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.borderSweep =
                 wholeIn(text, "algorithms.border-sweep", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.closed-form-columns",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.closedFormColumns =
                 wholeIn(text, "algorithms.closed-form-columns", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.infeasible-magnitude",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.infeasibleMagnitude = wholeIn(text, "algorithms.mc.infeasible-magnitude", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.infeasible-phase",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.infeasiblePhase = wholeIn(text, "algorithms.mc.infeasible-phase", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.feasible-magnitude",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.feasibleMagnitude = wholeIn(text, "algorithms.mc.feasible-magnitude", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.feasible-phase",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.feasiblePhase = wholeIn(text, "algorithms.mc.feasible-phase", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.best-gain",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.bestGain = wholeIn(text, "algorithms.mc.best-gain", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.tree-bisection",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.treeBisection = wholeIn(text, "algorithms.mc.tree-bisection", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.mc.stages",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.mc.stages = wholeIn(text, "algorithms.mc.stages", line, 0.0, 1.0) != 0.0;
         }},
        {"algorithms.local-search-budget",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.algorithms.localSearchBudget = static_cast<std::int32_t>(
                 wholeIn(text, "algorithms.local-search-budget", line, 1.0, 1.0e7));
         }},
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

        {"defaults.boundary-grid.from-cloud",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.boundariesFromCloud =
                 wholeIn(text, "defaults.boundary-grid.from-cloud", line, 0.0, 1.0) != 0.0;
         }},

        {"defaults.templates.point-count",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.templatePointCount = static_cast<std::int32_t>(
                 wholeIn(text, "defaults.templates.point-count", line, 1.0, 1.0e6));
         }},

        {"defaults.templates.epsilon-in-nichols",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.epsilonInNichols =
                 wholeIn(text, "defaults.templates.epsilon-in-nichols", line, 0.0, 1.0) != 0.0;
         }},
        {"defaults.templates.db-per-degree",
         [](const std::string & text, std::int64_t line, Settings & into) {
             into.defaults.dbPerDegree =
                 realIn(text, "defaults.templates.db-per-degree", line, 1.0e-6, 1.0e6);
         }},
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

}

Settings readSettings(const std::string & path)
{
    std::ifstream file(path);
    if (!file.good()) {
        throw FileError(QFTBX_TR("Core", "the settings file cannot be read: %1").arg(path));
    }

    Settings settings;
    settings.source = path;

    std::string section;
    std::string line;
    std::int64_t number = 0;
    std::vector<std::string> seen;

    while (std::getline(file, line)) {
        number++;

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
                throw InvalidInput(QFTBX_TR("Core", "settings, line %1: a section needs its closing bracket").arg(number));
            }

            section = trimmed(content.substr(1, content.size() - 2));
            if (section.empty()) {
                throw InvalidInput(QFTBX_TR("Core", "settings, line %1: a section needs a name").arg(number));
            }
            continue;
        }

        const std::size_t equals = content.find('=');
        if (equals == std::string::npos) {
            throw InvalidInput(QFTBX_TR("Core", "settings, line %1: expected \"key = value\", found \"%2\"").arg(number).arg(content));
        }

        const std::string name = trimmed(content.substr(0, equals));
        const std::string value = trimmed(content.substr(equals + 1));

        if (name.empty()) {
            throw InvalidInput(QFTBX_TR("Core", "settings, line %1: the key is missing").arg(number));
        }

        const std::string key = section.empty() ? name : section + "." + name;

        if (std::find(seen.begin(), seen.end(), key) != seen.end()) {
            throw InvalidInput(QFTBX_TR("Core", "settings, line %1: \"%2\" is set more than once").arg(number).arg(key));
        }
        seen.push_back(key);

        const auto & table = bindings();
        const auto found = std::find_if(table.begin(), table.end(),
                                        [&key](const Binding & binding) {
                                            return key == binding.key;
                                        });

        if (found == table.end()) {
            settings.unknownKeys.push_back(key);
            continue;
        }

        found->apply(value, number, settings);
    }

    return settings;
}

std::string userSettingsPath()
{
    return homePath();
}

void writeSetting(const std::string & path, const std::string & key, const std::string & value)
{
    const std::size_t dot = key.rfind('.');
    if (dot == std::string::npos) {
        throw InvalidInput(QFTBX_TR("Core", "a setting key needs its section, as in 'interface.language': '%1'").arg(key));
    }
    const std::string section = key.substr(0, dot);
    const std::string name = key.substr(dot + 1);

    std::vector<std::string> lines;
    {
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
    }

    std::string current;
    std::ptrdiff_t sectionStart = -1;
    std::ptrdiff_t sectionEnd = -1;
    std::ptrdiff_t keyLine = -1;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        std::string content = lines[i];
        const std::size_t comment = content.find_first_of("#;");
        if (comment != std::string::npos) {
            content = content.substr(0, comment);
        }
        content = trimmed(content);
        if (content.empty()) {
            continue;
        }
        if (content.front() == '[' && content.back() == ']') {
            if (current == section && sectionStart >= 0 && sectionEnd < 0) {
                sectionEnd = static_cast<std::ptrdiff_t>(i);
            }
            current = trimmed(content.substr(1, content.size() - 2));
            if (current == section && sectionStart < 0) {
                sectionStart = static_cast<std::ptrdiff_t>(i);
            }
            continue;
        }
        const std::size_t equals = content.find('=');
        if (equals != std::string::npos && current == section && trimmed(content.substr(0, equals)) == name) {
            keyLine = static_cast<std::ptrdiff_t>(i);
        }
    }
    if (sectionStart >= 0 && sectionEnd < 0) {
        sectionEnd = static_cast<std::ptrdiff_t>(lines.size());
    }

    const std::string entry = name + " = " + value;
    if (keyLine >= 0) {
        lines[static_cast<std::size_t>(keyLine)] = entry;
    } else if (sectionStart >= 0) {
        std::ptrdiff_t at = sectionEnd;
        while (at > sectionStart + 1 && trimmed(lines[static_cast<std::size_t>(at - 1)]).empty()) {
            --at;
        }
        lines.insert(lines.begin() + at, entry);
    } else {
        if (!lines.empty() && !trimmed(lines.back()).empty()) {
            lines.emplace_back();
        }
        lines.push_back("[" + section + "]");
        lines.push_back(entry);
    }

    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw FileError(QFTBX_TR("Core", "the settings file cannot be written: %1").arg(path));
    }
    for (const std::string & line : lines) {
        out << line << "\n";
    }
}

std::vector<std::string> settingKeys()
{
    std::vector<std::string> keys;
    for (const Binding & binding : bindings()) {
        keys.emplace_back(binding.key);
    }
    return keys;
}

void openRecord(const Settings & settings)
{
    if (!settings.log.enabled) {
        qftbx::record::close();
        return;
    }

    std::string path = settings.log.path;
    if (path.empty()) {
        const char * state = std::getenv("XDG_STATE_HOME");
        const char * home = std::getenv("HOME");
        if (state != nullptr && *state != '\0') {
            path = std::string(state) + "/qftbx/qftbx.log";
        } else if (home != nullptr) {
            path = std::string(home) + "/.local/state/qftbx/qftbx.log";
        } else {
            return;
        }
    }

    qftbx::record::open(path, std::size_t(settings.log.sizeLimitKilobytes) * 1024u);
}

Settings loadSettings()
{
    const std::string named = environmentPath();
    if (!named.empty()) {
        return readSettings(named);
    }

    for (const std::string & candidate : {std::string("qftbx.conf"), homePath()}) {
        if (readable(candidate)) {
            return readSettings(candidate);
        }
    }

    return Settings();
}

}
