#ifndef QFTBX_GUI_FREQUENCY_LEGEND_H
#define QFTBX_GUI_FREQUENCY_LEGEND_H

#include <QColor>
#include <QGroupBox>
#include <QString>
#include <QVector>

class QCheckBox;
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
 *   worked one at a time.
 */
class FrequencyLegend : public QGroupBox
{
    Q_OBJECT

public:
    /// One row: its container, its checkbox and the layout more controls
    /// can be added to.
    struct Row {
        QWidget * widget = nullptr;
        QCheckBox * check = nullptr;
        QVBoxLayout * layout = nullptr;
    };

    explicit FrequencyLegend(QWidget * parent = nullptr);

    /// Appends a checked row labelled 'text' in 'color'.
    Row addRow(const QString & text, const QColor & color);

    /// Destroys every row, ready for a replot.
    void clear();

    int rowCount() const { return m_checks.size(); }
    bool isRowChecked(int index) const;

signals:
    /// A row's checkbox was clicked, or all of them were: the owner
    /// re-applies the visibilities.
    void rowToggled();

private:
    void setAll(bool checked);
    /// Whether the row would be shown with the filter as it stands. Asked of
    /// the filter, not of the widget: a legend that is not on screen yet has
    /// no visible rows.
    bool passesFilter(int index) const;
    void applyFilter(const QString & text);

    QWidget * m_rowHolder = nullptr;
    QVBoxLayout * m_layout = nullptr;
    QLineEdit * m_filter = nullptr;

    //Observers: the rows are Qt children of the holder.
    QVector<QWidget *> m_rows;
    QVector<QCheckBox *> m_checks;
};

} // namespace qftbx

#endif // QFTBX_GUI_FREQUENCY_LEGEND_H
