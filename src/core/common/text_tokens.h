/**
 * @file
 * @brief The toolbox's one way of turning reals into text, and back.
 *
 * Declares the formatter every file and message uses for a real: the
 * shortest decimal that reads back as the same double, never shorter than
 * six significant digits so that 1000 stays "1000" and not "1e+03". A
 * second form rounds to a number of significant digits for what a form
 * shows, significant and not decimal so that a small margin does not print
 * as zero; it drops the trailing zeros of the rounding, and 'digits' below
 * one or above the seventeen that round-trip a double gives the full form.
 * Alongside them, joining, whitespace tokenising, and a parse of a whole
 * line of reals that gives nothing when any token is not a number, so a
 * malformed frequency file or coefficient list is rejected as a whole
 * rather than silently truncated.
 */

#ifndef QFTBX_TEXT_TOKENS_H
#define QFTBX_TEXT_TOKENS_H

#include <optional>
#include <string>

#include <string>
#include <vector>

namespace qftbx {
namespace text {

std::string number(double value);

std::string number(double value, int digits);

std::string join(const std::vector<std::string> & pieces, const std::string & separator);

std::vector<std::string> tokens(const std::string & line);

std::optional<std::vector<double>> reals(const std::string & line);

}
}

#endif
