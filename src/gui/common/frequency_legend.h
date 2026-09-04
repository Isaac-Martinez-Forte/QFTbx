#ifndef QFTBX_GUI_FREQUENCY_LEGEND_H
#define QFTBX_GUI_FREQUENCY_LEGEND_H

#include <QCheckBox>
#include <QColor>
#include <QGroupBox>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

namespace qftbx {

/**
 * @brief The box of colour-coded frequency rows beside a plot: one checkbox
 * per curve, in the curve's colour, to show or hide it.
 *
 * Five viewers built this box by hand, each with its own copy of the row
 * construction, the row deletion on replot and the visibility loop. The
 * rows belong to this widget: clear() destroys them, which is how a widget
 * leaves a layout in Qt, and a caller that needs more than a checkbox in a
 * row (the template viewer adds an epsilon slider and field) adds it to the
 * row's own layout.
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
    /// A row's checkbox was clicked: the owner re-applies the visibilities.
    void rowToggled();

private:
    QVBoxLayout * m_layout = nullptr;
    //Observers: the rows are Qt children of this box.
    QVector<QWidget *> m_rows;
    QVector<QCheckBox *> m_checks;
};

} // namespace qftbx

#endif // QFTBX_GUI_FREQUENCY_LEGEND_H
