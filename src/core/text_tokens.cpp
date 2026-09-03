#include "src/core/text_tokens.h"

#include <QRegularExpression>
#include <vector>
#include <cstdio>
#include <cstdlib>

namespace {

//17 significant digits always round-trip a double.
const int kMaxSignificantDigits = 17;

//Never fewer than this, which is what QString::number(double) used, so
//every value that already printed exactly keeps printing byte for byte the
//same text - and 1000 stays "1000" instead of becoming the shorter but
//worse "1e+03".
const int kMinSignificantDigits = 6;

} // namespace


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


std::vector<QString> qftbx::text::tokens(const QString & line){

    QStringList parts = line.split(" ");
    std::vector<QString> result;
    result.reserve(parts.size());

    for (const QString & part : parts){
        if (!part.isEmpty()){
            result.push_back(part);
        }
    }

    return result;
}


std::optional<std::vector<double>> qftbx::text::reals(const QString & line){

    //Split on any whitespace (spaces, tabs, newlines): frequency files
    //usually carry one value per line.
    QList<QString> parts = line.split(QRegularExpression("\\s+"),
                                                Qt::SkipEmptyParts);
    QList<double> values;

    bool ok = false;

    for (const QString & part : parts){
        const double value = part.toDouble(&ok);

        if (!ok){
            return std::nullopt;
        }

        values.push_back(value);
    }

    return std::vector<double>(values.begin(), values.end());
}



