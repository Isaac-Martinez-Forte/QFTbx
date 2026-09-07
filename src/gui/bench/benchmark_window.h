#ifndef QFTBX_GUI_BENCH_BENCHMARK_WINDOW_H
#define QFTBX_GUI_BENCH_BENCHMARK_WINDOW_H

#include <functional>

#include <QMainWindow>
#include <QString>

#include "src/bench/plan.h"

class QAction;
class QTabWidget;

namespace qftbx {

class BenchmarkRun;
class PlanEditor;
class QueueView;
class ResultsView;

/**
 * @brief The benchmark planner: a plan edited, saved, run and read in one
 * window.
 *
 * Three tabs, in the order of the work: the plan, the queue of cases as
 * they run, the results. The plan is saved to its file before it runs,
 * since the worker processes read it from there, with its paths relative
 * to that file so it travels with its project and its results. The same
 * plan file runs unattended with qftbx-bench on another machine.
 *
 * Opened from the main window's Tools menu; built only with
 * QFTBX_BUILD_BENCHMARK.
 */
class BenchmarkWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BenchmarkWindow(QWidget * parent = nullptr);
    ~BenchmarkWindow() override;

    /// How a file name gets asked for (the dialogs are modal and cannot be
    /// driven by a test): forSaving and a filter, returning the path or an
    /// empty string.
    using FileChooser = std::function<QString (bool forSaving, const QString & filter)>;
    void setFileChooser(FileChooser chooser);

    /// How a question gets asked (save before running?); true for yes.
    using Confirmer = std::function<bool (const QString & question)>;
    void setConfirmer(Confirmer confirmer) { m_confirm = std::move(confirmer); }

    PlanEditor * editor() const { return m_editor; }
    QueueView * queue() const { return m_queue; }
    ResultsView * results() const { return m_results; }

    QString planPath() const { return m_planPath; }
    bool isRunning() const;

    void newPlan();
    void openPlan(const QString & path);
    /// Saves to the current file, asking for one when there is none;
    /// returns false when the user gave none or the plan cannot be read.
    bool savePlan();
    bool savePlanAs();
    void run();
    void stop();
    /// Loads the records already on disk for the current plan into the
    /// results, which is how a run made elsewhere is read here.
    void loadResults();

private:
    void build();
    void updateCaseCount();
    void setPlanPath(const QString & path);
    QString chooseFile(bool forSaving, const QString & filter);
    bool confirm(const QString & question);
    bench::Plan currentPlanAbsolute() const;

    QTabWidget * m_tabs = nullptr;
    PlanEditor * m_editor = nullptr;
    QueueView * m_queue = nullptr;
    ResultsView * m_results = nullptr;
    BenchmarkRun * m_run = nullptr;

    QAction * m_runAction = nullptr;
    QAction * m_stopAction = nullptr;
    QAction * m_loadResultsAction = nullptr;

    QString m_planPath;
    FileChooser m_chooseFile;
    Confirmer m_confirm;
};

} // namespace qftbx

#endif // QFTBX_GUI_BENCH_BENCHMARK_WINDOW_H
