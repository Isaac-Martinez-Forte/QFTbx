/**
 * @file
 * @brief Reals as text for fields and labels, at full precision or at the
 * shown digits.
 *
 * Both go through the project's one formatter, qftbx::text::number, and
 * never QString::number(), which keeps six significant digits. A number the
 * user types or that goes to a file keeps every digit (numberText()), the
 * shortest text that reads back to the same double, so reopening a form
 * does not round what the file holds. A number the user only reads, a gain
 * off an optimisation or a coefficient inside a drawn formula, is shown by
 * shownText() at a global count of significant digits, four unless the
 * settings change it through setShownDigits(); one global, because it is
 * one answer for the whole interface. A level in decibels, a value set
 * against its bound or the margin between them, is shown by shownDbText()
 * at a fixed number of decimals, the shown digits less one: the difference
 * of two levels is exact to an absolute amount, not a relative one, so the
 * significant digits of a margin of 1e-12 dB say nothing. shownAsZeroDb()
 * tells a level that rounds to nothing there. withPowersOfTen() gives a
 * text as rich text with its numbers in exponent notation raised to powers
 * of ten.
 */

#ifndef QFTBX_GUI_NUMBER_TEXT_H
#define QFTBX_GUI_NUMBER_TEXT_H

#include <QString>

#include "src/core/common/text_tokens.h"

namespace qftbx {

inline QString numberText(double value)
{
    return QString::fromStdString(qftbx::text::number(value));
}

int shownDigits();
void setShownDigits(int digits);

inline QString shownText(double value)
{
    return QString::fromStdString(qftbx::text::number(value, shownDigits()));
}

double shownDbResolution();
QString shownDbText(double db);
bool shownAsZeroDb(double db);

QString withPowersOfTen(const QString & plain);

}

#endif
