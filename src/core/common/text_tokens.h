/**
 * @file
 * @brief The toolbox's one way of turning reals into text, and back.
 *
 * Declares the formatter every file and message uses for a real: the
 * shortest decimal that reads back as the same double, never shorter than
 * six significant digits so that 1000 stays "1000" and not "1e+03". A
 * second form rounds to a number of significant digits for what a form
 * shows, significant and not decimal so that a small margin does not print
 * as zero. Alongside them, joining, whitespace tokenising, and a parse of a
 * whole line of reals that rejects the line when any token is not a number.
 */

#ifndef QFTBX_TEXT_TOKENS_H
#define QFTBX_TEXT_TOKENS_H

#include <optional>
#include <string>

#include <string>
#include <vector>

namespace qftbx {
namespace text {

/**
 * @brief A real as text: the shortest decimal string that reads back as the
 * same double, but never shorter than the six significant digits the
 * toolbox has always printed.
 *
 * The one place the toolbox turns a real into text, so the answer to "how
 * many digits" is given once.
 *
 * The floor of six digits is deliberate and is what makes this change
 * safe: qftbx::text::number(double) used exactly six, so every value that
 * already printed exactly keeps printing byte for byte the same text -
 * 1000 is still "1000", 0.1 is still "0.1", 1e-16 is still "1e-16" - and
 * only the values that six digits could not represent change, which is the
 * whole point. Without the floor the shortest form of 1000 is "1e+03",
 * shorter and worse.
 *
 * Two traps this exists to close. Six significant digits truncate anything
 * needing more: 1234567.89 becomes "1.23457e+06". And std::to_string(double),
 * the obvious replacement, is six DECIMALS, which turns 1e-16 into
 * "0.000000" - a zero where a coefficient was.
 *
 * Implemented by asking for one more digit until the text round-trips,
 * because std::to_chars for floating point arrived in gcc 11 and this
 * builds on gcc 8. On a newer compiler that loop is a single to_chars call
 * and the result is the same string.
 */
std::string number(double value);

/**
 * @brief A real as text, rounded to the significant digits asked for.
 *
 * What a form SHOWS, as against what a file keeps. A gain read off an
 * optimisation is 567.3175312139062 and is stored that way; on screen it is
 * 567.3, because thirteen digits of a number the user is going to compare
 * by eye are noise. Significant digits and not decimals, so that the small
 * numbers survive: an excess of 6.988e-05 dB rounded to four DECIMALS is
 * "0.0000", a zero where a margin was.
 *
 * Trailing zeros of the rounding are dropped ("1.5000" is "1.5"), and
 * 'digits' below one, or above the seventeen that round-trip a double, is
 * the full number(double).
 */
std::string number(double value, int digits);

/**
 * @brief The pieces joined by a separator, as QStringList::join() did.
 *
 * Needed because std::string has no join and the alternative at every call
 * is a loop with a "first time round" flag.
 */
std::string join(const std::vector<std::string> & pieces, const std::string & separator);

/// Splits a string into its whitespace-separated tokens.
std::vector<std::string> tokens(const std::string & line);

/// Parses whitespace-separated reals, or nothing when any token is not a
/// valid real, so a malformed frequency file or coefficient list is
/// rejected as a whole rather than silently truncated.
std::optional<std::vector<double>> reals(const std::string & line);

}
}

#endif
