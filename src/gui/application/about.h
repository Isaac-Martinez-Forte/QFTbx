#ifndef QFTBX_GUI_ABOUT_H
#define QFTBX_GUI_ABOUT_H

#include <QString>

class QWidget;

namespace qftbx {

/// The text of the About box: what the toolbox is, who wrote it, that it
/// is under development, where its code and documentation are, the licence
/// and the libraries. Rich text, translated.
QString aboutText();

/// The About box over the given window.
void showAbout(QWidget * parent);

} // namespace qftbx

#endif // QFTBX_GUI_ABOUT_H
