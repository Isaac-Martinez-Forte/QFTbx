#ifndef QFTBX_GUI_NUMBER_TEXT_H
#define QFTBX_GUI_NUMBER_TEXT_H

#include <QString>

#include "src/core/text_tokens.h"

namespace qftbx {

/**
 * @brief A real as the text of a field or a label.
 *
 * Through qftbx::text::number, the one formatter of the project: the
 * shortest text that reads back to the same double. QString::number(double)
 * keeps six significant digits, and the dialogs paint stored values into
 * fields they read back on accept, so reopening a specification or a
 * template epsilon used to round what the file held.
 */
inline QString numberText(double value)
{
    return QString::fromStdString(qftbx::text::number(value));
}

} // namespace qftbx

#endif // QFTBX_GUI_NUMBER_TEXT_H
