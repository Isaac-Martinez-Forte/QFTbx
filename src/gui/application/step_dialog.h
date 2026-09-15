#ifndef QFTBX_STEP_DIALOG_H
#define QFTBX_STEP_DIALOG_H

#include <QDialog>

/**
 * @brief Base of the seven dialogs that describe one step of a design.
 *
 * It exists for one member: the accepted flag, which has to be CLEARED on
 * every run, since the window reuses a dialog between visits. Seven copies
 * of one boolean are seven chances to forget that.
 *
 * So from the first acceptance onwards wasAccepted() answered true for ever,
 * while takePlant() and its siblings had already handed the payload over.
 * Reopening the plant dialog and closing it with Escape came back to the
 * window with wasAccepted() true and published a NULL plant, which wiped the
 * plant from the project and the templates, boundaries and design with it.
 *
 * Here the flag means what its name says: the user accepted THIS showing.
 * The window clears it before showing a dialog again, which is a thing it can
 * only do if the seven share a type - hence this one.
 *
 * It is explicit and NOT a showEvent override on purpose: exec() would fire a
 * showEvent, but a test drives these through MainWindow::setDialogRunner and
 * never shows them, so clearing there would behave differently in a test than
 * in the application - which is the one thing a seam for testing must not do.
 */

namespace qftbx {

class StepDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StepDialog(QWidget * parent = nullptr) : QDialog(parent) {}

    /// Whether the user accepted THIS showing of the dialog.
    bool wasAccepted() const { return m_accepted; }

    /// Forgets a previous acceptance. Called before the dialog is shown again.
    void clearAcceptance() { m_accepted = false; }

protected:
    /// A subclass says the input it holds has been accepted.
    void markAccepted() { m_accepted = true; }

private:
    bool m_accepted = false;
};

} // namespace qftbx

#endif // QFTBX_STEP_DIALOG_H
