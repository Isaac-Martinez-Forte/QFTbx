/**
 * @file
 * @brief Construction and behaviour of the frequency legend.
 *
 * The filter sits over the two buttons rather than beside them, so it stays
 * wide enough to read its placeholder once the buttons are translated. All
 * and None act on what the filter is showing, asked of the filter and not
 * of the widgets, since the rows of a legend not yet on screen report
 * themselves invisible. Rows are given one width, the widest, when the box
 * is shown, after callers have added their own controls. Setting a row from
 * code does not emit the toggle signal, which is connected to the click and
 * not to the state.
 */

#include "src/gui/common/frequency_legend.h"

#include <algorithm>

#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "src/gui/common/field_mark.h"

namespace qftbx {

FrequencyLegend::FrequencyLegend(QWidget * parent)
    : QGroupBox(parent)
{
    setObjectName("frequenciesBox");
    setTitle(tr("Frequencies"));

    QVBoxLayout * outer = new QVBoxLayout(this);
    outer->setSpacing(4);

    QGridLayout * controls = new QGridLayout();
    controls->setSpacing(4);

    m_filter = new QLineEdit(this);
    m_filter->setObjectName("legendFilter");
    m_filter->setPlaceholderText(tr("filter"));
    m_filter->setToolTip(tr("Shows only the frequencies whose number contains this text."));
    m_filter->setClearButtonEnabled(true);
    connect(m_filter, &QLineEdit::textChanged, this, &FrequencyLegend::applyFilter);
    controls->addWidget(m_filter, 0, 0, 1, 2);

    QPushButton * all = new QPushButton(tr("All"), this);
    all->setObjectName("legendAll");
    all->setToolTip(tr("Ticks every frequency the filter is showing."));
    QPushButton * none = new QPushButton(tr("None"), this);
    none->setObjectName("legendNone");
    none->setToolTip(tr("Unticks them."));
    connect(all, &QPushButton::clicked, this, [this]() { setAll(true); });
    connect(none, &QPushButton::clicked, this, [this]() { setAll(false); });
    controls->addWidget(all, 1, 0);
    controls->addWidget(none, 1, 1);
    outer->addLayout(controls);

    m_controls = controls;

    QScrollArea * scroll = new QScrollArea(this);
    scroll->setObjectName("legendScroll");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    m_rowHolder = new QWidget(scroll);
    m_layout = new FlowLayout(m_rowHolder, 0, 4);
    scroll->setWidget(m_rowHolder);
    outer->addWidget(scroll);
}

void FrequencyLegend::setBare(bool bare)
{
    m_bare = bare;

    setTitle(bare ? QString() : tr("Frequencies"));

    markAs(this, "bare", bare);

    for (int i = 0; i < m_controls->count(); ++i) {
        if (QWidget * control = m_controls->itemAt(i)->widget()) {
            control->setVisible(!bare);
        }
    }
}

FrequencyLegend::Row FrequencyLegend::addRow(const QString & text, const QColor & color)
{
    Row row;
    row.widget = new QWidget(m_rowHolder);
    row.widget->setObjectName("row");

    row.column = new QVBoxLayout(row.widget);
    row.column->setContentsMargins(0, 0, 0, 0);
    row.column->setSpacing(1);

    row.layout = new QHBoxLayout();
    row.layout->setContentsMargins(0, 0, 0, 0);
    row.layout->setSpacing(4);
    row.column->addLayout(row.layout);

    row.check = new QCheckBox(row.widget);
    row.check->setObjectName("check");
    row.check->setText(text);
    row.check->setStyleSheet("color : " + color.name());
    row.check->setCheckState(Qt::Checked);
    if (!m_bare) {
        row.check->setToolTip(tr("Shows or hides what belongs to this frequency, in rad/s."));
    }
    row.layout->addWidget(row.check);

    m_layout->addWidget(row.widget);
    m_rows.push_back(row.widget);
    m_checks.push_back(row.check);

    connect(row.check, &QCheckBox::clicked, this, &FrequencyLegend::rowToggled);

    return row;
}

void FrequencyLegend::clear()
{
    for (QWidget * row : m_rows) {
        delete row;
    }
    m_rows.clear();
    m_checks.clear();
    if (m_filter != nullptr) {
        m_filter->clear();
    }
}

void FrequencyLegend::showEvent(QShowEvent * event)
{
    QGroupBox::showEvent(event);
    tidyWidths();
}

void FrequencyLegend::tidyWidths()
{
    int widest = 0;
    for (QWidget * row : m_rows) {
        widest = std::max(widest, row->sizeHint().width());
    }

    for (QWidget * row : m_rows) {
        row->setMinimumWidth(widest);
    }
}

bool FrequencyLegend::passesFilter(int index) const
{
    const QString wanted = m_filter == nullptr ? QString() : m_filter->text().trimmed();
    return wanted.isEmpty() ||
            m_checks.at(index)->text().contains(wanted, Qt::CaseInsensitive);
}

void FrequencyLegend::setAll(bool checked)
{
    bool changed = false;
    for (int i = 0; i < m_checks.size(); ++i) {
        if (passesFilter(i) && m_checks.at(i)->isChecked() != checked) {
            m_checks.at(i)->setChecked(checked);
            changed = true;
        }
    }
    if (changed) {
        emit rowToggled();
    }
}

void FrequencyLegend::applyFilter(const QString & text)
{
    const QString wanted = text.trimmed();
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows.at(i)->setVisible(wanted.isEmpty() ||
                                 m_checks.at(i)->text().contains(wanted, Qt::CaseInsensitive));
    }
}

void FrequencyLegend::setRowChecked(int index, bool checked)
{
    if (index >= 0 && index < m_checks.size()) {
        m_checks.at(index)->setChecked(checked);
    }
}

bool FrequencyLegend::isRowChecked(int index) const
{
    return index >= 0 && index < m_checks.size() &&
            m_checks.at(index)->checkState() != Qt::Unchecked;
}

}
