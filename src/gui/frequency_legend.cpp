#include "src/gui/frequency_legend.h"

namespace qftbx {

FrequencyLegend::FrequencyLegend(QWidget * parent)
    : QGroupBox(parent)
{
    setObjectName("frequenciesBox");
    setTitle(tr("Frequencies"));
}

FrequencyLegend::Row FrequencyLegend::addRow(const QString & text, const QColor & color)
{
    //A widget holds exactly one layout: it is built with the first row and
    //destroyed with the rows.
    if (m_layout == nullptr) {
        m_layout = new QVBoxLayout(this);
    }

    Row row;
    row.widget = new QWidget(this);
    row.widget->setObjectName("row");
    row.layout = new QVBoxLayout(row.widget);
    row.layout->setContentsMargins(0, 0, 0, 0);

    row.check = new QCheckBox(row.widget);
    row.check->setObjectName("check");
    row.check->setText(text);
    row.check->setStyleSheet("color : " + color.name());
    row.check->setCheckState(Qt::Checked);
    row.layout->addWidget(row.check);

    m_layout->addWidget(row.widget);
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

    delete m_layout;
    m_layout = nullptr;
}

bool FrequencyLegend::isRowChecked(int index) const
{
    return index >= 0 && index < m_checks.size() &&
            m_checks.at(index)->checkState() != Qt::Unchecked;
}

} // namespace qftbx
