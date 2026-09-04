#ifndef QFTBX_SETTINGS_H
#define QFTBX_SETTINGS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace qftbx {

/**
 * @brief The values a user may change without recompiling.
 *
 * Plain fields with the compiled defaults, grouped the way the file is. NOT a
 * map looked up by name: some of these are read once per node of an interval
 * search, where a string lookup would cost a hundred times what the value is
 * worth. Whoever needs one COPIES it when it is constructed, so the hot path
 * reads a member.
 *
 * Loaded once at startup and immutable afterwards, which is also what makes
 * it safe next to OpenMP and the search's worker thread: there is nothing to
 * synchronise on something nobody writes.
 *
 * What is deliberately NOT here: the mathematical and structural constants.
 * 2*pi, the two layers of the boundary union, the seven specification slots
 * and the 0 dB ray of the stability criterion can all be written down in a
 * file, and writing them down would not configure anything - it would break
 * the program. A setting is a value that has a defensible range.
 */
struct Settings {

    /**
     * @brief Ceilings that exist to stop a typo, not to express a limit of
     * the method.
     *
     * Every one of these only ever REFUSES input, so moving them changes no
     * computed result - which is why they are the safest thing in the file.
     */
    struct Limits {
        /// Cells of the Nichols grid for the boundaries, phase x magnitude.
        /// A one-degree grid over 360 degrees is 361 points per axis, so ten
        /// million cells is far past anything sensible.
        std::int64_t maxGridCells = 10000000;

        /// Points per parameter grid in the template sweep. The count comes
        /// from an expression the user types, and a negative one used to ask
        /// for 1.8e19 doubles.
        double maxTemplatePoints = 1.0e6;

        /// Design frequencies in one set.
        std::int32_t maxFrequencyCount = 1000000;

        /// Largest magnitude accepted in the loop-shaping dialog's fields,
        /// which is what keeps an infinity out of an integer conversion.
        double maxMagnitude = 1.0e12;
    } limits;

    /**
     * @brief What the interval search is allowed to spend.
     */
    struct Search {
        /// Live nodes the branch & bound list may hold before it refuses to
        /// grow. It is a MEMORY budget: a node measures 528 bytes with two
        /// parameters and 1056 with eight, so the default is of the order of
        /// 17 to 34 GB. Every run reports its peak next to the elapsed time,
        /// and those peaks are what this should be set from.
        std::size_t maxLiveNodes = 32000000;
    } search;

    /// The path this was read from, empty when nothing was read and the
    /// compiled defaults stand. Reported rather than guessed at: on a shared
    /// machine the interesting question is usually WHICH file is in effect.
    std::string source;

    /// Keys the file carried that this build does not know. A warning and
    /// not an error, so a file written by a later version still starts.
    std::vector<std::string> unknownKeys;
};

/**
 * @brief Reads the settings from an explicit path.
 *
 * Throws qftbx::InvalidInput naming the key, the line and the range when a
 * value cannot be used, and qftbx::FileError when the path cannot be read.
 * A value that is not a number does NOT become zero: silently turning bad
 * input into a plausible number is the defect this project has spent the most
 * time removing.
 */
Settings readSettings(const std::string & path);

/**
 * @brief Reads the settings from the first file that exists, in order:
 *
 *   1. the path in the QFTBX_CONFIG environment variable, when set - which
 *      is how a variant gets tried on a shared machine without touching
 *      anyone's home directory;
 *   2. ./qftbx.conf, next to wherever the program was started;
 *   3. $HOME/.config/qftbx/qftbx.conf.
 *
 * WITH NO FILE AT ALL IT SUCCEEDS, returning the compiled defaults with an
 * empty source. That is not a convenience: it is what makes adding this
 * unable to break an installation that never had one.
 *
 * A file named by QFTBX_CONFIG that cannot be read IS an error, because
 * naming it says it is meant to be used.
 */
Settings loadSettings();

} // namespace qftbx

#endif // QFTBX_SETTINGS_H
