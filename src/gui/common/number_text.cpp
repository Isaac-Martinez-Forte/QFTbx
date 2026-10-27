/**
 * @file
 * @brief The global count of digits a form shows, and numbers as rich text.
 *
 * Four by default, which is what a gain, a margin or a coefficient is read
 * at, and three decimals of a decibel. The project file is never written
 * through it. withPowersOfTen() finds the numbers in exponent notation in a
 * text, a whole number or a decimal followed by e and the exponent, and
 * writes each as its mantissa times ten raised to the exponent, the way the
 * drawn formulas show them.
 */

#include "src/gui/common/number_text.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

#include "src/core/math/formula.h"

namespace qftbx {

namespace {

int g_shownDigits = 4;

}

int shownDigits()
{
    return g_shownDigits;
}

void setShownDigits(int digits)
{
    g_shownDigits = digits;
}

double shownDbResolution()
{
    return std::pow(10.0, -std::max(0, g_shownDigits - 1));
}

QString shownDbText(double db)
{
    if (!std::isfinite(db)) {
        return shownText(db);
    }
    if (shownAsZeroDb(db)) {
        return QStringLiteral("0");
    }
    QString text = QString::number(db, 'f', std::max(0, g_shownDigits - 1));
    if (text.contains('.')) {
        while (text.endsWith('0')) {
            text.chop(1);
        }
        if (text.endsWith('.')) {
            text.chop(1);
        }
    }
    return text;
}

bool shownAsZeroDb(double db)
{
    return std::abs(db) < 0.5 * shownDbResolution();
}

QString withPowersOfTen(const QString & plain)
{
    static const QRegularExpression exponentNotation(QStringLiteral("(?<![\\w.])-?\\d+(?:\\.\\d+)?[eE][+-]?\\d+(?![\\w.])"));

    const QString escaped = plain.toHtmlEscaped();
    QString out;
    qsizetype last = 0;
    QRegularExpressionMatchIterator found = exponentNotation.globalMatch(escaped);
    while (found.hasNext()) {
        const QRegularExpressionMatch match = found.next();
        std::string mantissa;
        std::string exponent;
        if (!formula::splitExponent(match.captured().toStdString(), mantissa, exponent)) {
            continue;
        }
        const QString raised = "10<sup>" + QString::fromStdString(exponent).replace('-', QChar(0x2212)) + "</sup>";
        out += escaped.mid(last, match.capturedStart() - last);
        out += mantissa == "1" ? raised
             : mantissa == "-1" ? "-" + raised
             : QString::fromStdString(mantissa) + QChar(0x00b7) + raised;
        last = match.capturedEnd();
    }
    return out + escaped.mid(last);
}

}
