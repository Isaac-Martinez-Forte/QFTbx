/**
 * @file
 * @brief Number formatting by round trip, and whole-token parsing.
 *
 * The shortest round-tripping text is found by printing with %g at six
 * significant digits and asking for one more until strtod reads the value
 * back exactly, up to the seventeen that always suffice. Rounding to a
 * digit count prints at that precision and reprints the result at its own
 * length, which drops the trailing zeros %g keeps and leaves the exponent
 * alone. A token is a real only when strtod consumes all of it, so a
 * partial parse rejects the whole line.
 */

#include "src/core/common/text_tokens.h"

#include <string>
#include <vector>
#include <cstdio>
#include <sstream>
#include <cstdlib>

namespace {

const int kMaxSignificantDigits = 17;

const int kMinSignificantDigits = 6;

}

std::string qftbx::text::join(const std::vector<std::string> & pieces,
                              const std::string & separator)
{
    std::string text;

    for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (i != 0) {
            text += separator;
        }
        text += pieces[i];
    }

    return text;
}

std::string qftbx::text::number(double value)
{
    char buffer[64];

    for (int digits = kMinSignificantDigits; digits < kMaxSignificantDigits; ++digits) {
        std::snprintf(buffer, sizeof buffer, "%.*g", digits, value);
        if (std::strtod(buffer, nullptr) == value) {
            return buffer;
        }
    }

    std::snprintf(buffer, sizeof buffer, "%.*g", kMaxSignificantDigits, value);
    return buffer;
}

std::string qftbx::text::number(double value, int digits)
{
    if (digits < 1 || digits >= kMaxSignificantDigits) {
        return number(value);
    }

    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%.*g", digits, value);

    return number(std::strtod(buffer, nullptr));
}

std::vector<std::string> qftbx::text::tokens(const std::string & line){

    std::vector<std::string> result;
    std::istringstream stream (line);

    for (std::string part; stream >> part; ){
        result.push_back(part);
    }

    return result;
}

std::optional<std::vector<double>> qftbx::text::reals(const std::string & line){

    const std::vector<std::string> parts = tokens(line);
    std::vector<double> values;
    values.reserve(parts.size());

    for (const std::string & part : parts){
        char * end = nullptr;
        const double value = std::strtod(part.c_str(), &end);
        const bool ok = end != nullptr && *end == '\0' && end != part.c_str();

        if (!ok){
            return std::nullopt;
        }

        values.push_back(value);
    }

    return values;
}
