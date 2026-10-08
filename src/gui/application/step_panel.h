/**
 * @file
 * @brief Base of the seven forms that describe one step of a design.
 *
 * A plain widget rather than a dialog: each form lives inside the card of
 * its phase and several are open at once. The base holds the two things
 * the forms share: an accepted flag, cleared with clearAcceptance() before
 * every visit because the window reuses a form and would otherwise publish
 * a payload already handed over, so wasAccepted() answers for this visit
 * only; and the accepted() signal, emitted by markAccepted(), by which a
 * form says the user pressed its button with valid data. The window
 * listens and does the rest; a form knows nothing about the project.
 */

#ifndef QFTBX_STEP_PANEL_H
#define QFTBX_STEP_PANEL_H

#include <QWidget>

namespace qftbx {

class StepPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StepPanel(QWidget * parent = nullptr) : QWidget(parent) {}

    bool wasAccepted() const { return m_accepted; }

    void clearAcceptance() { m_accepted = false; }

signals:
    void accepted();

protected:
    void markAccepted()
    {
        m_accepted = true;
        emit accepted();
    }

private:
    bool m_accepted = false;
};

}

#endif
