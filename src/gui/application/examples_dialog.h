/**
 * @file
 * @brief The window that offers the design problems shipped with the
 * program.
 *
 * Declares the dialog that lists what the installation carries, shows what
 * each problem is and hands back the one to open. Nothing is loaded by
 * picking: the project is read only when the button is pressed, so the
 * problems can be read through without waiting for any of them. The
 * menu opens this same window with one of them already showing, the first
 * when it names none the directory holds. The dialog owns nothing of the
 * project: chosenPath() is the file accepted, empty when it was dismissed,
 * and the window opens it as it opens any other file.
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

class ExamplesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExamplesDialog(QWidget * parent = nullptr,
                            const QString & selected = QString());

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
