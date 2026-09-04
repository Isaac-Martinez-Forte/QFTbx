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

    /**
     * @brief The resolution of the nominal stability check.
     *
     * These trade TIME against how reliably the check decides, and they do
     * not touch the criterion itself: the Cohen-Chait-Yaniv count and its
     * 0 dB ray are not settings, they are the method.
     *
     * What they govern is the frequency grid the nominal loop is sampled on
     * before the crossings are counted. Too coarse and a fast phase turn is
     * missed, and the checker answers "cannot decide", which conservatively
     * discards a candidate that may have been fine.
     */
    struct Stability {
        /// Points of the base logarithmic grid.
        std::int32_t baseGridPoints = 3000;

        /// Decades sampled beyond the design frequencies, on both sides.
        double decadesBeyond = 3.0;

        /// Phase step, in degrees, above which the grid is refined. It is the
        /// unwrapping tolerance: a turn faster than this between two samples
        /// cannot be unwrapped safely.
        double maxPhaseStepDegrees = 30.0;

        /// Refinements one verdict may spend before giving up.
        std::int32_t refinementBudget = 200000;
    } stability;

    /**
     * @brief Numbers that come from the published algorithms.
     *
     * THE GROUP TO BE CAREFUL WITH, and the reason it says so here: unlike
     * everything above, these change WHAT THE ALGORITHM COMPUTES, not how
     * long it takes to compute it. Each one is a figure from a paper, and the
     * provenance is written next to it because a value changed here makes the
     * golden tests and the article validations stop describing the program
     * that is running.
     *
     * They are exposed anyway, deliberately: a doctoral toolbox whose
     * published parameters can only be explored by recompiling is a worse
     * tool. What makes it safe is the rule that no test reads the settings
     * file - every one of them builds its own, so a value here can never
     * change what a test means.
     */
    struct Algorithms {
        /// MR (Rambabu & Nataraj, FDA-10): template points entering the
        /// constraint set per frequency, paired quadratically for the
        /// tracking constraint. The paper uses 9.
        /// KNOWN LIMIT, measured on Example 5.1: the certified design exceeds
        /// the true tracking bound by 12 to 19 per cent at two of the five
        /// frequencies. Raising this to 25 shrinks the excess from 0.456 to
        /// 0.431 dB at twelve times the cost, without removing it - the way
        /// out is to bound the spread by intervals instead of sampling it,
        /// which is a modelling decision and not a setting.
        std::int32_t templateRepresentatives = 9;

        /// MR: passes of the HC4 narrowing before a box is accepted as
        /// narrowed no further.
        std::int32_t maxNarrowingPasses = 8;

        /// NK (Nataraj & Kubal 2007): iterations the local refinement of a
        /// candidate point may spend.
        std::int32_t localSearchBudget = 400;

        /// NK: the ratio at which the gain bisection stops, so 1.01 is one
        /// per cent. It is a PRUNING bound, not the answer's accuracy.
        double gainTolerance = 1.01;

        /// MC1 (Martinez-Forte & Cervera, IJRNC 2021): the same ratio for the
        /// certified gain search.
        double certifiedGainTolerance = 1.01;
    } algorithms;

    /**
     * @brief What the dialogs start with.
     *
     * The zero-risk group: these are what a field is PREFILLED with, so
     * nothing here can change a computed result - the user sees the value and
     * can still type over it. It is also the group that shows most in daily
     * use: whoever always works with the same Nichols grid should not have to
     * type it again every time.
     */
    struct Defaults {
        /// The Nichols grid of the boundary computation: the phase axis in
        /// degrees, the magnitude axis in decibels, and the points on each.
        double phaseStart = -360.0;
        double phaseEnd = 0.0;
        std::int32_t phasePoints = 361;
        double magnitudeStart = -60.0;
        double magnitudeEnd = 60.0;
        std::int32_t magnitudePoints = 121;

        /// Points per parameter grid in the template sweep.
        std::int32_t templatePointCount = 10;

        /// The frequency range the loop-shaping plot starts with, in rad/s,
        /// and how many points over it.
        ///
        /// One range and not one per mode. The dialog used to carry THREE
        /// hardcoded sets - one on opening, one on picking linear, one on
        /// picking logarithmic - and the last two differed from each other
        /// and from the first with no reason given anywhere. Worse, they
        /// overwrote whatever was in the fields, so a configured default
        /// would have been thrown away the moment a mode was picked.
        double loopStart = 1.0e-9;
        double loopEnd = 10.0;
        std::int32_t loopPointCount = 100;
    } defaults;

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
