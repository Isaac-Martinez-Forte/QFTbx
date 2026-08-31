#ifndef QFTBX_GUI_ERROR_MESSAGE_H
#define QFTBX_GUI_ERROR_MESSAGE_H

#include <QMessageBox>
#include <QString>

//GUI-side error dialog, moved out of the backend (tools.h). Note for the
//phase-7 review: the dialog has no parent (not modal to the main window) and
//several call sites swap message and title.
namespace tools {

inline void errorMessage(QString mensaje, QString nombre){

    QMessageBox::critical(nullptr, nombre, mensaje,
                          QMessageBox::Close);
}

} // namespace tools

#endif // QFTBX_GUI_ERROR_MESSAGE_H
