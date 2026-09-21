/**
 * @file
 * @brief GUI-side error reporting and the translation of core messages.
 *
 * By default an error opens a modal dialog, which is right for a user and
 * impossible for an automated run, so the destination can be replaced and
 * a test collects what the dialogs would have said. The header also
 * declares how a core message or an exception becomes text in the language
 * of the interface: a toolbox exception carries its text and its arguments
 * apart, so the text is translated and the arguments put back; a parse
 * error gets its file and line around the translated message; any other
 * exception is shown as it is.
 */

#ifndef QFTBX_GUI_ERROR_MESSAGE_H
#define QFTBX_GUI_ERROR_MESSAGE_H

#include <functional>

#include <QString>

#include <exception>

#include "src/core/common/message.h"

namespace qftbx {

using ErrorReporter = std::function<void (const QString & message, const QString & title)>;

/// Sends an error to the current destination (a modal dialog by default).
void errorMessage(QString message, QString title);

/// Replaces the destination; a default-constructed reporter restores the
/// dialog. Returns the previous one, so a caller can put it back.
ErrorReporter setErrorReporter(ErrorReporter reporter);

/// What an exception says, in the language of the interface. A
/// qftbx::Exception carries its text and its arguments apart, so the text
/// is translated (context "Core" of the translation file) and the
/// arguments put back; a parse error gets its file and line around the
/// translated message; any other exception is shown as it is.
QString translated(const std::exception & failure);

/// A core message in the language of the interface.
QString translated(const Message & message);

}

#endif
