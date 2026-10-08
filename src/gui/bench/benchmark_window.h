/**
 * @file
 * @brief The benchmark planner window: a plan edited, saved, run and read.
 *
 * Declares the main window that holds the plan editor, the queue of cases
 * and the results as three tabs, in the order of the work, with a toolbar
 * for new, open, save, run, stop and reading results already on disk. The
 * plan is saved to its file before it runs, since the worker processes
 * read it from there, with its paths relative to that file so it travels
 * with its project and its results; the same file runs unattended with
 * qftbx-bench on another machine, and loadResults() reads the records such
 * a run left. savePlan() asks for a file when there is none and returns
 * false when the user gave none or the plan cannot be read. File and
 * confirmation dialogs are behind replaceable seams so a test can drive
 * the window: the chooser returns a path or an empty string, the confirmer
 * true for yes. Built only with QFTBX_BUILD_BENCHMARK and opened from the
 * main window's Tools menu.
 */

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

class BenchmarkWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BenchmarkWindow(QWidget * parent = nullptr);
    ~BenchmarkWindow() override;

    using FileChooser = std::function<QString (bool forSaving, const QString & filter)>;
    void setFileChooser(FileChooser chooser);

    using Confirmer = std::function<bool (const QString & question)>;
    void setConfirmer(Confirmer confirmer) { m_confirm = std::move(confirmer); }

    PlanEditor * editor() const { return m_editor; }
    QueueView * queue() const { return m_queue; }
    ResultsView * results() const { return m_results; }

    QString planPath() const { return m_planPath; }
    bool isRunning() const;

    void newPlan();
    void openPlan(const QString & path);
    bool savePlan();
    bool savePlanAs();
    void run();
    void stop();
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

}

#endif
