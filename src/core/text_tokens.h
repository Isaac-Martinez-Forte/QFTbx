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
 * Two traps this exists to close. Six significant digits silently truncated
 * anything needing more: 1234567.89 was written "1.23457e+06". And
 * std::to_string(double) - the obvious replacement when Qt goes - is six
 * DECIMALS, which turns 1e-16 into "0.000000", a zero where a coefficient
 * used to be.
 *
 * Implemented by asking for one more digit until the text round-trips,
 * because std::to_chars for floating point arrived in gcc 11 and this
 * builds on gcc 8. On a newer compiler that loop is a single to_chars call
 * and the result is the same string.
 */
std::string number(double value);

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

} // namespace text
} // namespace qftbx

#endif // QFTBX_TEXT_TOKENS_H
