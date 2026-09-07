#ifndef QFTBX_GUI_BENCH_QUEUE_VIEW_H
#define QFTBX_GUI_BENCH_QUEUE_VIEW_H

#include <cstddef>
#include <vector>

#include <QWidget>

#include "src/bench/plan.h"
#include "src/bench/record.h"

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QTableWidget;

namespace qftbx {

/**
 * @brief The cases of a run, one row each, with their state as they go.
 */
class QueueView : public QWidget
{
    Q_OBJECT

public:
    explicit QueueView(QWidget * parent = nullptr);

    void setCases(const bench::Plan & plan, const std::vector<bench::Case> & cases);
    void markStarted(const bench::Case & c);
    void markFinished(const bench::Case & c, const bench::Record & record);
    void finish(std::size_t failures, const QString & error);
    void log(const QString & line);

    std::size_t doneCount() const { return m_done; }

private:
    QTableWidget * m_table = nullptr;
    QProgressBar * m_progress = nullptr;
    QLabel * m_status = nullptr;
    QPlainTextEdit * m_log = nullptr;
    std::size_t m_done = 0;
    std::size_t m_total = 0;
};

} // namespace qftbx

#endif // QFTBX_GUI_BENCH_QUEUE_VIEW_H
