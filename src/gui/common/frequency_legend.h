/**
 * @file
 * @brief The colour-coded box of frequency rows beside a plot.
 *
 * One checkbox per curve, in the curve's colour, shows or hides it; a row
 * is added checked. The rows flow into as many columns as the width allows
 * and live in a scroll area, since a problem has as many rows as design
 * frequencies and the last ones must stay reachable; a filter and All and
 * None buttons work them together. A caller that needs more than a tick in
 * a row adds it to the row's own line, or to the column under it, after
 * addRow has returned, which is why the rows are given the width of the
 * widest only when the box is shown. The bare mode keeps only the ticks,
 * for where the box is not a legend but the question of which frequencies
 * a specification applies at. The rows belong to the widget and are
 * destroyed when it is cleared.
 *
 * rowToggled reports what the user does, a click on a row or on All or
 * None, and the owner re-applies the visibilities; setRowChecked writes
 * the legend without emitting it. The filter is asked whether a row
 * passes, not the row whether it is visible, since a legend not yet on
 * screen has no visible rows.
 */

#ifndef QFTBX_GUI_FREQUENCY_LEGEND_H
#define QFTBX_GUI_FREQUENCY_LEGEND_H

#include <QColor>
#include <QGroupBox>
#include <QString>
#include <QVector>

#include "src/gui/common/flow_layout.h"

class QCheckBox;
class QGridLayout;
class QHBoxLayout;
class QLineEdit;
class QVBoxLayout;
class QWidget;

namespace qftbx {

class FrequencyLegend : public QGroupBox
{
    Q_OBJECT

public:
    struct Row {
        QWidget * widget = nullptr;
        QCheckBox * check = nullptr;
        QHBoxLayout * layout = nullptr;
        QVBoxLayout * column = nullptr;
    };

    explicit FrequencyLegend(QWidget * parent = nullptr);

    Row addRow(const QString & text, const QColor & color);

    void setBare(bool bare);

    void clear();

    int rowCount() const { return m_checks.size(); }
    bool isRowChecked(int index) const;

    void setRowChecked(int index, bool checked);

signals:
    void rowToggled();

protected:
    void showEvent(QShowEvent * event) override;

private:
    void setAll(bool checked);
    void tidyWidths();
    bool passesFilter(int index) const;
    void applyFilter(const QString & text);

    QGridLayout * m_controls = nullptr;

    bool m_bare = false;

    QWidget * m_rowHolder = nullptr;
    FlowLayout * m_layout = nullptr;
    QLineEdit * m_filter = nullptr;

    QVector<QWidget *> m_rows;
    QVector<QCheckBox *> m_checks;
};

}

#endif
