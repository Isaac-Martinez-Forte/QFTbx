#include "src/core/text_tokens.h"

#include <string>
#include <vector>
#include <cstdio>
#include <sstream>
#include <cstdlib>

namespace {

//17 significant digits always round-trip a double.
const int kMaxSignificantDigits = 17;

//Never fewer than this, which is what qftbx::text::number(double) used, so
//every value that already printed exactly keeps printing byte for byte the
//same text - and 1000 stays "1000" instead of becoming the shorter but
//worse "1e+03".
const int kMinSignificantDigits = 6;

} // namespace


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


std::vector<std::string> qftbx::text::tokens(const std::string & line){

    std::vector<std::string> result;
    std::istringstream stream (line);

    //Whitespace-separated, which is what the frequency files and the
    //coefficient lists are. split(" ") plus an empty-piece filter was the
    //same thing said in two steps, and it treated a tab as content.
    for (std::string part; stream >> part; ){
        result.push_back(part);
    }

    return result;
}


std::optional<std::vector<double>> qftbx::text::reals(const std::string & line){

    //Whitespace-separated, which is what tokens() already does: frequency
    //files usually carry one value per line.
    const std::vector<std::string> parts = tokens(line);
    std::vector<double> values;
    values.reserve(parts.size());

    for (const std::string & part : parts){
        //strtod plus the end pointer is how "the WHOLE token is a real" is
        //asked in the standard library: a partial parse is a rejection, as
        //QString::toDouble's ok flag was.
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



