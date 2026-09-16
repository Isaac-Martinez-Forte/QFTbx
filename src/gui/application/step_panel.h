#ifndef QFTBX_STEP_PANEL_H
#define QFTBX_STEP_PANEL_H

#include <QWidget>

/**
 * @brief Base of the seven forms that describe one step of a design.
 *
 * A plain widget, not a dialog: each of these lives inside a dock of the
 * main window, beside the diagram of its own step, and several are open at
 * once. As dialogs they were modal - the form disappeared the moment the
 * computation it asked for began, and nothing else could be looked at
 * while one was open.
 *
 * It exists for two members. The accepted flag, which has to be CLEARED on
 * every visit, since the window reuses a form: from the first acceptance
 * onwards wasAccepted() answered true for ever, while takePlant() and its
 * siblings had already handed the payload over, so closing a reopened form
 * published a NULL plant and wiped the step from the project. Seven copies
 * of one boolean are seven chances to forget that.
 *
 * And the accepted() signal, which is how a form says the user pressed its
 * button with valid data in it. The window listens and does the rest: a
 * form still knows nothing about the project.
 */

namespace qftbx {

class StepPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StepPanel(QWidget * parent = nullptr) : QWidget(parent) {}

    /// Whether the user accepted THIS visit of the form.
    bool wasAccepted() const { return m_accepted; }

    /// Forgets a previous acceptance. Called before the form is filled again.
    void clearAcceptance() { m_accepted = false; }

signals:
    /// The user accepted what the form holds, and it is valid.
    void accepted();

protected:
    /// A subclass says the input it holds has been accepted.
    void markAccepted()
    {
        m_accepted = true;
        emit accepted();
    }

private:
    bool m_accepted = false;
};

} // namespace qftbx

#endif // QFTBX_STEP_PANEL_H
