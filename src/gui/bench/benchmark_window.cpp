#include "src/gui/bench/benchmark_window.h"

#include <exception>

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>

#include "src/bench/measurement.h"
#include "src/bench/record.h"
#include "src/bench/runner.h"
#include "src/bench/summary.h"
#include "src/core/common/exception.h"
#include "src/gui/application/error_message.h"
#include "src/gui/bench/benchmark_run.h"
#include "src/gui/bench/plan_editor.h"
#include "src/gui/bench/queue_view.h"
#include "src/gui/bench/results_view.h"

namespace qftbx {

namespace {

const QString kPlanFilter = QStringLiteral("Benchmark plans (*.qftbench *.xml)");

//A plan's paths as written to its file: relative to the file, so the plan
//moves with its project and its results.
bench::Plan relativeTo(bench::Plan plan, const QString & planPath)
{
    const QDir base = QFileInfo(planPath).absoluteDir();
    plan.projectFile = base.relativeFilePath(QString::fromStdString(plan.projectFile)).toStdString();
    plan.outputDirectory = base.relativeFilePath(QString::fromStdString(plan.outputDirectory)).toStdString();
    return plan;
}

bench::Plan absoluteFrom(bench::Plan plan, const QString & planPath)
{
    const QDir base = QFileInfo(planPath).absoluteDir();
    plan.projectFile = QDir::cleanPath(base.absoluteFilePath(QString::fromStdString(plan.projectFile))).toStdString();
    plan.outputDirectory = QDir::cleanPath(base.absoluteFilePath(QString::fromStdString(plan.outputDirectory))).toStdString();
    return plan;
}

} // namespace

BenchmarkWindow::BenchmarkWindow(QWidget * parent) : QMainWindow(parent)
{
    build();
    setPlanPath(QString());
    updateCaseCount();
}

BenchmarkWindow::~BenchmarkWindow() = default;

void BenchmarkWindow::build()
{
    setWindowTitle(tr("Benchmark planner"));
    resize(1000, 760);

    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);
    m_editor = new PlanEditor(m_tabs);
    m_queue = new QueueView(m_tabs);
    m_results = new ResultsView(m_tabs);
    m_tabs->addTab(m_editor, tr("Plan"));
    m_tabs->addTab(m_queue, tr("Queue"));
    m_tabs->addTab(m_results, tr("Results"));

    QToolBar * bar = addToolBar(tr("Plan"));
    bar->setMovable(false);
    QAction * newAction = bar->addAction(tr("New"));
    newAction->setObjectName("actionNewPlan");
    QAction * openAction = bar->addAction(tr("Open..."));
    openAction->setObjectName("actionOpenPlan");
    QAction * saveAction = bar->addAction(tr("Save"));
    saveAction->setObjectName("actionSavePlan");
    QAction * saveAsAction = bar->addAction(tr("Save as..."));
    saveAsAction->setObjectName("actionSavePlanAs");
    bar->addSeparator();
    m_runAction = bar->addAction(tr("Run"));
    m_runAction->setObjectName("actionRun");
    m_stopAction = bar->addAction(tr("Stop"));
    m_stopAction->setObjectName("actionStop");
    m_stopAction->setEnabled(false);
    bar->addSeparator();
    m_loadResultsAction = bar->addAction(tr("Load results"));
    m_loadResultsAction->setObjectName("actionLoadResults");
    m_loadResultsAction->setToolTip(tr("Read the records already on disk for this plan: a run made with qftbx-bench elsewhere"));

    m_run = new BenchmarkRun(this);
    m_run->setHandlers(
        [this](const bench::Case & c) { m_queue->markStarted(c); },
        [this](const bench::Case & c, const bench::Record & record) { m_queue->markFinished(c, record); },
        [this](std::size_t failures, const QString & error) {
            m_queue->finish(failures, error);
            m_runAction->setEnabled(true);
            m_stopAction->setEnabled(false);
            if (!error.isEmpty()) {
                errorMessage(error, tr("Benchmark"));
            }
            loadResults();
            m_tabs->setCurrentWidget(m_results);
            statusBar()->showMessage(error.isEmpty() ? tr("Run finished") : tr("Run stopped"));
        });

    m_editor->setChangeHandler([this]() { updateCaseCount(); });
    m_editor->setFileChooser([this](bool forSaving, bool directory, const QString & filter) {
        if (directory && !m_chooseFile) {
            return QFileDialog::getExistingDirectory(this, tr("Output directory"));
        }
        return chooseFile(forSaving, filter);
    });
    m_results->setFileChooser([this](bool forSaving, const QString & filter) { return chooseFile(forSaving, filter); });

    connect(newAction, &QAction::triggered, this, [this]() { newPlan(); });
    connect(openAction, &QAction::triggered, this, [this]() {
        const QString path = chooseFile(false, kPlanFilter);
        if (!path.isEmpty()) {
            openPlan(path);
        }
    });
    connect(saveAction, &QAction::triggered, this, [this]() { savePlan(); });
    connect(saveAsAction, &QAction::triggered, this, [this]() { savePlanAs(); });
    connect(m_runAction, &QAction::triggered, this, [this]() { run(); });
    connect(m_stopAction, &QAction::triggered, this, [this]() { stop(); });
    connect(m_loadResultsAction, &QAction::triggered, this, [this]() { loadResults(); });

    statusBar();
}

void BenchmarkWindow::setFileChooser(FileChooser chooser)
{
    m_chooseFile = std::move(chooser);
}

QString BenchmarkWindow::chooseFile(bool forSaving, const QString & filter)
{
    if (m_chooseFile) {
        return m_chooseFile(forSaving, filter);
    }
    return forSaving ? QFileDialog::getSaveFileName(this, tr("Save the plan"), m_planPath, filter)
                     : QFileDialog::getOpenFileName(this, tr("Open a plan"), m_planPath, filter);
}

bool BenchmarkWindow::confirm(const QString & question)
{
    if (m_confirm) {
        return m_confirm(question);
    }
    return QMessageBox::question(this, tr("Benchmark"), question) == QMessageBox::Yes;
}

bool BenchmarkWindow::isRunning() const
{
    return m_run->isRunning();
}

void BenchmarkWindow::setPlanPath(const QString & path)
{
    m_planPath = path;
    setWindowTitle(path.isEmpty() ? tr("Benchmark planner") : tr("Benchmark planner - %1").arg(QFileInfo(path).fileName()));
    m_loadResultsAction->setEnabled(!path.isEmpty());
}

void BenchmarkWindow::updateCaseCount()
{
    try {
        const bench::Plan plan = m_editor->plan();
        const std::size_t cases = bench::expandCases(plan).size();
        statusBar()->showMessage(tr("%1 cases: %2 structures x %3 algorithms x %4 epsilons x %5 repetitions%6")
                                 .arg(cases).arg(bench::measuredStructures(plan).size()).arg(plan.algorithms.size())
                                 .arg(plan.epsilons.size()).arg(plan.repetitions)
                                 .arg(plan.warmUp ? tr(" plus a warm-up") : QString()));
    } catch (const std::exception & incomplete) {
        statusBar()->showMessage(QString::fromUtf8(incomplete.what()));
    }
}

void BenchmarkWindow::newPlan()
{
    if (isRunning()) {
        errorMessage(tr("A run is in progress; stop it first."), tr("Benchmark"));
        return;
    }
    m_editor->setPlan(bench::examplePlan());
    m_queue->setCases(bench::examplePlan(), {});
    m_results->clear();
    setPlanPath(QString());
    updateCaseCount();
}

void BenchmarkWindow::openPlan(const QString & path)
{
    try {
        m_editor->setPlan(absoluteFrom(bench::readPlan(path.toStdString()), path));
        setPlanPath(path);
        m_results->clear();
        updateCaseCount();
        loadResults();
    } catch (const std::exception & failure) {
        errorMessage(QString::fromUtf8(failure.what()), tr("Open a plan"));
    }
}

bench::Plan BenchmarkWindow::currentPlanAbsolute() const
{
    return m_editor->plan();
}

bool BenchmarkWindow::savePlanAs()
{
    const QString path = chooseFile(true, kPlanFilter);
    if (path.isEmpty()) {
        return false;
    }
    setPlanPath(path);
    return savePlan();
}

bool BenchmarkWindow::savePlan()
{
    if (m_planPath.isEmpty()) {
        return savePlanAs();
    }
    try {
        const bench::Plan plan = currentPlanAbsolute();
        bench::writePlan(relativeTo(plan, m_planPath), m_planPath.toStdString());
        statusBar()->showMessage(tr("Plan saved to %1").arg(m_planPath));
        return true;
    } catch (const std::exception & failure) {
        errorMessage(QString::fromUtf8(failure.what()), tr("Save the plan"));
        return false;
    }
}

void BenchmarkWindow::run()
{
    if (isRunning()) {
        return;
    }
    bench::Plan plan;
    try {
        plan = currentPlanAbsolute();
    } catch (const std::exception & incomplete) {
        errorMessage(QString::fromUtf8(incomplete.what()), tr("Run"));
        return;
    }

    //The worker processes read the plan from its file.
    if (m_planPath.isEmpty() && !confirm(tr("The plan has to be saved before it runs. Save it now?"))) {
        return;
    }
    if (!savePlan()) {
        return;
    }

    try {
        const std::vector<bench::Case> cases = bench::expandCases(plan);
        m_queue->setCases(plan, cases);
        m_results->clear();
        m_run->start(plan, m_planPath, 0);
        m_runAction->setEnabled(false);
        m_stopAction->setEnabled(true);
        m_tabs->setCurrentWidget(m_queue);
        statusBar()->showMessage(tr("Running %1 cases").arg(cases.size()));
    } catch (const std::exception & failure) {
        errorMessage(QString::fromUtf8(failure.what()), tr("Run"));
    }
}

void BenchmarkWindow::stop()
{
    if (isRunning()) {
        m_run->cancel();
        statusBar()->showMessage(tr("Stopping: the running cases are being killed"));
    }
}

void BenchmarkWindow::loadResults()
{
    if (m_planPath.isEmpty()) {
        return;
    }
    try {
        const bench::Plan plan = currentPlanAbsolute();
        const std::string records = bench::recordsDirectory(plan);
        if (!QDir(QString::fromStdString(records)).exists()) {
            m_results->clear();
            return;
        }
        m_results->show(bench::summarize(bench::readRecords(records)));
    } catch (const std::exception & failure) {
        errorMessage(QString::fromUtf8(failure.what()), tr("Results"));
    }
}

} // namespace qftbx
