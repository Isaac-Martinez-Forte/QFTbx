#include "src/gui/common/frequency_legend.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace qftbx {

FrequencyLegend::FrequencyLegend(QWidget * parent)
    : QGroupBox(parent)
{
    setObjectName("frequenciesBox");
    setTitle(tr("Frequencies"));

    QVBoxLayout * outer = new QVBoxLayout(this);

    m_filter = new QLineEdit(this);
    m_filter->setObjectName("legendFilter");
    m_filter->setPlaceholderText(tr("filter"));
    m_filter->setClearButtonEnabled(true);
    connect(m_filter, &QLineEdit::textChanged, this, &FrequencyLegend::applyFilter);
    outer->addWidget(m_filter);

    QHBoxLayout * buttons = new QHBoxLayout();
    QPushButton * all = new QPushButton(tr("All"), this);
    all->setObjectName("legendAll");
    QPushButton * none = new QPushButton(tr("None"), this);
    none->setObjectName("legendNone");
    connect(all, &QPushButton::clicked, this, [this]() { setAll(true); });
    connect(none, &QPushButton::clicked, this, [this]() { setAll(false); });
    buttons->addWidget(all);
    buttons->addWidget(none);
    outer->addLayout(buttons);

    //The rows scroll: there are as many as the problem has frequencies, and
    //the box is as tall as the window leaves it.
    QScrollArea * scroll = new QScrollArea(this);
    scroll->setObjectName("legendScroll");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    m_rowHolder = new QWidget(scroll);
    m_layout = new QVBoxLayout(m_rowHolder);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addStretch();
    scroll->setWidget(m_rowHolder);
    outer->addWidget(scroll);
}

FrequencyLegend::Row FrequencyLegend::addRow(const QString & text, const QColor & color)
{
    Row row;
    row.widget = new QWidget(m_rowHolder);
    row.widget->setObjectName("row");
    row.layout = new QVBoxLayout(row.widget);
    row.layout->setContentsMargins(0, 0, 0, 0);

    row.check = new QCheckBox(row.widget);
    row.check->setObjectName("check");
    row.check->setText(text);
    row.check->setStyleSheet("color : " + color.name());
    row.check->setCheckState(Qt::Checked);
    row.layout->addWidget(row.check);

    //Before the stretch that keeps the rows at the top.
    m_layout->insertWidget(m_layout->count() - 1, row.widget);
    m_rows.push_back(row.widget);
    m_checks.push_back(row.check);

    connect(row.check, &QCheckBox::clicked, this, &FrequencyLegend::rowToggled);

    return row;
}

void FrequencyLegend::clear()
{
    //Qt's own mechanism: destroying the row widget is how it leaves the
    //layout, and it takes its controls with it.
    for (QWidget * row : m_rows) {
        delete row;
    }
    m_rows.clear();
    m_checks.clear();
    if (m_filter != nullptr) {
        m_filter->clear();
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
        //What the FILTER is showing, asked of the filter and not of the
        //widget: a row of a legend that has not been shown yet answers that
        //it is not visible, and All would then do nothing at all.
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

bool FrequencyLegend::isRowChecked(int index) const
{
    return index >= 0 && index < m_checks.size() &&
            m_checks.at(index)->checkState() != Qt::Unchecked;
}

} // namespace qftbx
