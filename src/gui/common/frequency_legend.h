#ifndef QFTBX_GUI_FREQUENCY_LEGEND_H
#define QFTBX_GUI_FREQUENCY_LEGEND_H

#include <QColor>
#include <QGroupBox>
#include <QString>
#include <QVector>

#include "src/gui/common/flow_layout.h"

class QCheckBox;
class QHBoxLayout;
class QLineEdit;
class QVBoxLayout;
class QWidget;

namespace qftbx {

/**
 * @brief The box of colour-coded frequency rows beside a plot: one checkbox
 * per curve, in the curve's colour, to show or hide it.
 *
 * Things to keep in mind:
 * - The rows belong to this widget: clear() destroys them, which is how a
 *   widget leaves a layout in Qt, and a caller that needs more than a
 *   checkbox in a row (the template viewer adds an epsilon slider and field)
 *   adds it to the row's own layout.
 * - The rows live inside a SCROLL AREA. A design frequency is a row, and a
 *   problem has as many as it has frequencies - twenty-three on the
 *   Horowitz-Sidi motor, and twice that in the both-diagrams mode of the
 *   loop viewer - so without one the last rows are simply unreachable.
 * - All, none and a text filter are there because twenty checkboxes are not
 *   worked one at a time. They share one line: the box is a legend beside a
 *   diagram, and every pixel it takes is a pixel the diagram does not have.
 * - The rows FLOW into as many columns as the width allows. A frequency is
 *   four characters and a tick, and in one column five of them filled the
 *   height of a card while the diagram beside them was squeezed into
 *   nothing.
 */
class FrequencyLegend : public QGroupBox
{
    Q_OBJECT

public:
    /// One row: its container, its checkbox, the LINE the checkbox is on -
    /// where a caller puts what fits beside it - and the column under that
    /// line, for what does not.
    ///
    /// The line matters: the templates hang a slider and a field off every
    /// frequency, and stacked under it each row was three lines tall, so
    /// five frequencies filled a card while the diagram beside them had no
    /// room left.
    struct Row {
        QWidget * widget = nullptr;
        QCheckBox * check = nullptr;
        QHBoxLayout * layout = nullptr;
        QVBoxLayout * column = nullptr;
    };

    explicit FrequencyLegend(QWidget * parent = nullptr);

    /// Appends a checked row labelled 'text' in 'color'.
    Row addRow(const QString & text, const QColor & color);

    /**
     * @brief Just the ticks: no frame, no title, and none of the three
     * controls above them.
     *
     * For where the legend is not a legend but a question with the design
     * frequencies as its answers - which of them a specification applies at
     * - and the filter, All and None would be furniture around six tick
     * boxes.
     */
    void setBare(bool bare);

    /// Destroys every row, ready for a replot.
    void clear();

    int rowCount() const { return m_checks.size(); }
    bool isRowChecked(int index) const;

    /// Ticks or unticks a row without saying so: the caller is writing the
    /// legend, not answering it, and rowToggled is for what the user does.
    void setRowChecked(int index, bool checked);

signals:
    /// A row's checkbox was clicked, or all of them were: the owner
    /// re-applies the visibilities.
    void rowToggled();

protected:
    /// The rows are given one width, the widest of them, so that the
    /// columns line up. Here and not in addRow, because a caller that adds
    /// its own controls to a row does it after addRow has returned.
    void showEvent(QShowEvent * event) override;

private:
    void setAll(bool checked);
    void tidyWidths();
    /// Whether the row would be shown with the filter as it stands. Asked of
    /// the filter, not of the widget: a legend that is not on screen yet has
    /// no visible rows.
    bool passesFilter(int index) const;
    void applyFilter(const QString & text);

    //The filter and the two buttons above the rows, hidden by setBare().
    QHBoxLayout * m_controls = nullptr;

    QWidget * m_rowHolder = nullptr;
    FlowLayout * m_layout = nullptr;
    QLineEdit * m_filter = nullptr;

    //Observers: the rows are Qt children of the holder.
    QVector<QWidget *> m_rows;
    QVector<QCheckBox *> m_checks;
};

} // namespace qftbx

#endif // QFTBX_GUI_FREQUENCY_LEGEND_H
