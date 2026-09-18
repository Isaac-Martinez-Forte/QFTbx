#ifndef QFTBX_GUI_NUMBER_TEXT_H
#define QFTBX_GUI_NUMBER_TEXT_H

#include <QString>

#include "src/core/common/text_tokens.h"

namespace qftbx {

/**
 * @brief A real as the text of a field or a label.
 *
 * Through qftbx::text::number, the one formatter of the project: the
 * shortest text that reads back to the same double. QString::number(double)
 * keeps six significant digits, and the dialogs paint stored values into
 * fields they read back on accept, so reopening a specification or a
 * template epsilon must not round what the file holds.
 */
inline QString numberText(double value)
{
    return QString::fromStdString(qftbx::text::number(value));
}

/**
 * @brief How many significant digits a form SHOWS, and the same real at
 * that many.
 *
 * The other half of the rule above. A field the user types into, and every
 * number written to a file, keeps all the digits; a number the user only
 * READS - a gain off an optimisation, the excess of a verdict, a
 * coefficient inside a drawn formula - is shown at four, or at whatever the
 * settings say. One global, because it is one answer for the whole
 * interface and threading it through every viewer would say the same thing
 * two hundred times.
 */
int shownDigits();
void setShownDigits(int digits);

inline QString shownText(double value)
{
    return QString::fromStdString(qftbx::text::number(value, shownDigits()));
}

} // namespace qftbx

#endif // QFTBX_GUI_NUMBER_TEXT_H
