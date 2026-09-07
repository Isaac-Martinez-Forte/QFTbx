#ifndef QFTBX_GUI_ERROR_MESSAGE_H
#define QFTBX_GUI_ERROR_MESSAGE_H

#include <functional>

#include <QString>

#include <exception>

#include "src/core/common/message.h"

//GUI-side error reporting.
//
//By default a message opens a modal dialog, which is right for a user and
//impossible for an automated run: a headless suite would block forever on
//it. setReporter() replaces the destination, so a test can collect what
//the dialogs report and assert on it.
//
//The default dialog has no parent, so it is not modal to the main window.
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

} // namespace qftbx

#endif // QFTBX_GUI_ERROR_MESSAGE_H
