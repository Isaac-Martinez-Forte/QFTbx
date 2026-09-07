#include "src/gui/bench/queue_view.h"

#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QSplitter>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>

namespace qftbx {

namespace {

enum Column { Index, CaseId, Structure, Algorithm, Epsilon, Repetition, Status, Time, Gain, Memory, ColumnCount };

QTableWidgetItem * readOnly(const QString & text)
{
    auto * item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

} // namespace

QueueView::QueueView(QWidget * parent) : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    auto * splitter = new QSplitter(Qt::Vertical, this);
    layout->addWidget(splitter);

    m_table = new QTableWidget(0, ColumnCount, splitter);
    m_table->setObjectName("queue");
    m_table->setHorizontalHeaderLabels({tr("#"), tr("Case"), tr("Structure"), tr("Algorithm"), tr("Epsilon"),
                                        tr("Repetition"), tr("Status"), tr("Time (s)"), tr("k"), tr("Memory (MB)")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(m_table);

    m_log = new QPlainTextEdit(splitter);
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(5000);
    splitter->addWidget(m_log);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    m_progress = new QProgressBar(this);
    m_progress->setObjectName("progress");
    layout->addWidget(m_progress);
    m_status = new QLabel(this);
    m_status->setObjectName("queueStatus");
    layout->addWidget(m_status);
}

void QueueView::setCases(const bench::Plan & plan, const std::vector<bench::Case> & cases)
{
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(cases.size()));
    for (const bench::Case & c : cases) {
        const int row = static_cast<int>(c.index);
        m_table->setItem(row, Index, readOnly(QString::number(c.index)));
        m_table->setItem(row, CaseId, readOnly(QString::fromStdString(bench::caseId(c))));
        m_table->setItem(row, Structure, readOnly(QString::fromStdString(bench::structureLabel(plan, c.stepsApplied))));
        m_table->setItem(row, Algorithm, readOnly(QString::fromLatin1(bench::algorithmName(c.algorithm))));
        m_table->setItem(row, Epsilon, readOnly(QString::number(c.epsilon)));
        m_table->setItem(row, Repetition, readOnly(c.warmUp ? tr("warm-up") : QString::number(c.repetition)));
        m_table->setItem(row, Status, readOnly(tr("queued")));
        for (int column : {Time, Gain, Memory}) {
            m_table->setItem(row, column, readOnly(QString()));
        }
    }
    m_done = 0;
    m_total = cases.size();
    m_progress->setRange(0, static_cast<int>(m_total));
    m_progress->setValue(0);
    m_status->setText(tr("%1 cases queued").arg(m_total));
    m_log->clear();
}

void QueueView::markStarted(const bench::Case & c)
{
    const int row = static_cast<int>(c.index);
    if (row < m_table->rowCount()) {
        m_table->item(row, Status)->setText(tr("running"));
        m_table->scrollToItem(m_table->item(row, Status));
    }
}

void QueueView::markFinished(const bench::Case & c, const bench::Record & record)
{
    const int row = static_cast<int>(c.index);
    if (row < m_table->rowCount()) {
        m_table->item(row, Status)->setText(QString::fromStdString(record.status));
        if (record.status == "solved") {
            m_table->item(row, Time)->setText(QString::number(record.wallMilliseconds / 1000.0, 'f', 3));
            m_table->item(row, Gain)->setText(QString::number(record.gain, 'g', 8));
            if (record.peakMemoryBytes > 0) {
                m_table->item(row, Memory)->setText(QString::number(record.peakMemoryBytes / (1024.0 * 1024.0), 'f', 1));
            }
        } else {
            m_table->item(row, Time)->setText(QString::fromStdString(record.message));
        }
    }
    ++m_done;
    m_progress->setValue(static_cast<int>(m_done));
    m_status->setText(tr("%1 of %2 cases done").arg(m_done).arg(m_total));
    log(QStringLiteral("%1  %2: %3").arg(QString::fromStdString(record.caseId), QString::fromStdString(record.status),
                                          record.status == "solved" ? QStringLiteral("k=%1, %2 s").arg(record.gain).arg(record.wallMilliseconds / 1000.0)
                                                                    : QString::fromStdString(record.message)));
}

void QueueView::finish(std::size_t failures, const QString & error)
{
    if (!error.isEmpty()) {
        m_status->setText(tr("The run stopped: %1").arg(error));
        log(tr("stopped: %1").arg(error));
        return;
    }
    m_status->setText(failures == 0 ? tr("Done: %1 cases, all solved or infeasible").arg(m_total)
                                    : tr("Done: %1 cases, %2 failed").arg(m_total).arg(failures));
    log(tr("done"));
}

void QueueView::log(const QString & line)
{
    m_log->appendPlainText(QTime::currentTime().toString("HH:mm:ss") + "  " + line);
}

} // namespace qftbx
