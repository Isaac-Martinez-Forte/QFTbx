#ifndef QFTBX_GUI_ERROR_MESSAGE_H
#define QFTBX_GUI_ERROR_MESSAGE_H

#include <functional>

#include <QString>

//GUI-side error reporting, moved out of the backend (tools.h).
//
//By default a message opens a modal dialog, which is right for a user and
//impossible for an automated run: a headless suite would block forever on
//it. setReporter() replaces the destination, so a test can collect what
//the dialogs report and assert on it.
//
//The default dialog has no parent, so it is not modal to the main window.
namespace tools {

using ErrorReporter = std::function<void (const QString & message, const QString & title)>;

/// Sends an error to the current destination (a modal dialog by default).
void errorMessage(QString message, QString title);

/// Replaces the destination; a default-constructed reporter restores the
/// dialog. Returns the previous one, so a caller can put it back.
ErrorReporter setErrorReporter(ErrorReporter reporter);

} // namespace tools

#endif // QFTBX_GUI_ERROR_MESSAGE_H
