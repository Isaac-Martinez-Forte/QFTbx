/**
 * @file
 * @brief The window that offers the design problems shipped with the program.
 *
 * Declares the dialog that lists what the installation carries, shows what
 * each problem is and hands back the one to open. Nothing is loaded by
 * picking: the project is read only when the button is pressed, so the
 * twelve problems can be read through without waiting for any of them. The
 * menu opens this same window with one of them already showing.
 */

#ifndef QFTBX_GUI_EXAMPLES_DIALOG_H
#define QFTBX_GUI_EXAMPLES_DIALOG_H

#include <vector>

#include <QDialog>
#include <QString>

#include "src/gui/common/examples_library.h"

class QLabel;
class QListWidget;
class QPushButton;

namespace qftbx {

/**
 * @brief Lists the shipped examples and returns the one chosen.
 *
 * The dialog owns nothing of the project: it answers with a path and the
 * window opens it, so choosing an example and opening a file are the same
 * road from there on.
 */
class ExamplesDialog : public QDialog
{
    Q_OBJECT

public:
    /// `selected` is the file to show on opening; the first one when it
    /// names none or names one the directory does not hold.
    explicit ExamplesDialog(QWidget * parent = nullptr,
                            const QString & selected = QString());

    /// The file the user accepted, empty when the dialog was dismissed.
    QString chosenPath() const { return m_chosen; }

private:
    void showDescriptionOf(int row);
    void acceptRow(int row);

    std::vector<Example> m_examples;
    QListWidget * m_list = nullptr;
    QLabel * m_title = nullptr;
    QLabel * m_description = nullptr;
    QLabel * m_source = nullptr;
    QPushButton * m_open = nullptr;
    QString m_chosen;
};

}

#endif
