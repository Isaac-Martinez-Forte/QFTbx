// Smoke tests of the benchmark planner, headless: the window builds, the
// plan editor gives back what it was given, the queue and the results
// views take their data.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <QAction>
#include <QLabel>
#include <QTableWidget>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QThread>

#include "src/bench/plan.h"
#include "src/bench/summary.h"
#include "src/gui/bench/benchmark_run.h"
#include "src/gui/bench/benchmark_window.h"
#include "src/gui/bench/plan_editor.h"
#include "src/gui/bench/queue_view.h"
#include "src/gui/bench/results_view.h"
#include "src/gui/application/main_window.h"

using namespace qftbx;

namespace {

bench::Plan aPlan()
{
    bench::Plan plan = bench::examplePlan();
    plan.name = "planner";
    plan.projectFile = std::string(QFTBX_TEST_DATA_DIR "/acc90.qft");
    plan.outputDirectory = "/tmp/planner-results";
    plan.algorithms = {nt, mr};
    plan.epsilons = {0.5, 2.0};
    plan.repetitions = 3;
    plan.warmUp = false;
    plan.gainRange = Range(1.0, 2000.0);
    plan.steps = {{bench::StructureStep::Kind::Pole, 0.5, 500.0, false}, {bench::StructureStep::Kind::Zero, 0.1, 100.0, true}};
    plan.measures.memoryTrace = true;
    plan.jobs = 3;
    plan.timeoutSeconds = 120;
    plan.settings = {{"algorithms.mr-nichols-epsilon", "1"}};
    return plan;
}

} // namespace

TEST(BenchmarkPlanner, TheEditorGivesBackThePlanItWasGiven)
{
    BenchmarkWindow window;
    const bench::Plan given = aPlan();
    window.editor()->setPlan(given);
    const bench::Plan back = window.editor()->plan();

    EXPECT_EQ(back.name, given.name);
    EXPECT_EQ(back.projectFile, given.projectFile);
    EXPECT_EQ(back.outputDirectory, given.outputDirectory);
    EXPECT_EQ(back.algorithms, given.algorithms);
    EXPECT_EQ(back.epsilons, given.epsilons);
    EXPECT_EQ(back.repetitions, 3);
    EXPECT_FALSE(back.warmUp);
    ASSERT_TRUE(back.gainRange.has_value());
    EXPECT_DOUBLE_EQ(back.gainRange->max, 2000.0);
    ASSERT_EQ(back.steps.size(), 2u);
    EXPECT_EQ(back.steps[0].kind, bench::StructureStep::Kind::Pole);
    EXPECT_FALSE(back.steps[0].run);
    EXPECT_EQ(back.steps[1].kind, bench::StructureStep::Kind::Zero);
    EXPECT_DOUBLE_EQ(back.steps[1].max, 100.0);
    EXPECT_TRUE(back.measures.memoryTrace);
    EXPECT_EQ(back.jobs, 3);
    EXPECT_DOUBLE_EQ(back.timeoutSeconds, 120.0);
    ASSERT_EQ(back.settings.size(), 1u);
    EXPECT_EQ(back.settings[0].first, "algorithms.mr-nichols-epsilon");

    auto * structures = window.editor()->findChild<QLabel *>("structuresSummary");
    ASSERT_NE(structures, nullptr);
    EXPECT_TRUE(structures->text().contains("base+p+z")) << structures->text().toStdString();
    auto * summary = window.editor()->findChild<QLabel *>("projectSummary");
    ASSERT_NE(summary, nullptr);
    EXPECT_TRUE(summary->text().contains("boundaries")) << summary->text().toStdString();
}

TEST(BenchmarkPlanner, AnIncompletePlanIsRefusedWithItsReason)
{
    BenchmarkWindow window;
    bench::Plan plan = aPlan();
    plan.algorithms.clear();
    window.editor()->setPlan(plan);
    EXPECT_THROW(window.editor()->plan(), InvalidInput);
}

TEST(BenchmarkPlanner, SavingAndOpeningKeepThePathsRelativeToThePlan)
{
    QTemporaryDir directory;
    const QString path = directory.filePath("plan.qftbench");
    BenchmarkWindow window;
    window.setFileChooser([&](bool, const QString &) { return path; });
    bench::Plan plan = aPlan();
    plan.outputDirectory = directory.filePath("results").toStdString();
    window.editor()->setPlan(plan);
    ASSERT_TRUE(window.savePlan());
    EXPECT_EQ(window.planPath(), path);

    const bench::Plan onDisk = bench::readPlan(path.toStdString());
    EXPECT_EQ(onDisk.outputDirectory, "results") << "relative to the plan file";

    BenchmarkWindow other;
    other.openPlan(path);
    EXPECT_EQ(other.editor()->plan().outputDirectory, plan.outputDirectory) << "absolute again in the editor";
    EXPECT_EQ(other.editor()->plan().projectFile, plan.projectFile);
}

TEST(BenchmarkPlanner, TheQueueAndTheResultsTakeTheirRows)
{
    BenchmarkWindow window;
    const bench::Plan plan = aPlan();
    const std::vector<bench::Case> cases = bench::expandCases(plan);
    window.queue()->setCases(plan, cases);
    auto * queue = window.queue()->findChild<QTableWidget *>("queue");
    ASSERT_NE(queue, nullptr);
    EXPECT_EQ(queue->rowCount(), static_cast<int>(cases.size()));

    bench::Record record;
    record.caseId = bench::caseId(cases[0]);
    record.status = "solved";
    record.gain = 1000.0;
    record.wallMilliseconds = 12.5;
    window.queue()->markStarted(cases[0]);
    window.queue()->markFinished(cases[0], record);
    EXPECT_EQ(queue->item(0, 6)->text(), "solved");
    EXPECT_EQ(window.queue()->doneCount(), 1u);

    std::vector<bench::Aggregate> aggregates(2);
    aggregates[0].structure = "base";
    aggregates[0].algorithm = "nt";
    aggregates[0].epsilon = 0.5;
    aggregates[0].solved = aggregates[0].runs = 3;
    aggregates[0].wallMilliseconds.median = 40.0;
    aggregates[1] = aggregates[0];
    aggregates[1].stepsApplied = 2;
    aggregates[1].structure = "base+p+z";
    aggregates[1].wallMilliseconds.median = 400.0;
    window.results()->show(aggregates);
    EXPECT_EQ(window.results()->rowCount(), 2);
}

TEST(BenchmarkPlanner, TheMainWindowOffersItUnderTools)
{
    MainWindow window;
    auto * action = window.findChild<QAction *>("actionBenchmarkPlanner");
    ASSERT_NE(action, nullptr);
    action->trigger();
    auto * planner = window.findChild<BenchmarkWindow *>();
    ASSERT_NE(planner, nullptr);
    EXPECT_FALSE(planner->isRunning());
}

TEST(BenchmarkPlanner, ARunFromTheWindowFillsTheQueueAndTheResults)
{
    //The whole path: the plan saved, the worker processes launched by the
    //runner on its thread, the events delivered on the GUI thread, the
    //records read back into the results.
    BenchmarkRun::setWorkerProgram(QStringLiteral(QFTBX_BENCH_TOOL));
    QTemporaryDir directory;
    const QString path = directory.filePath("run.qftbench");

    BenchmarkWindow window;
    window.setFileChooser([&](bool, const QString &) { return path; });
    window.setConfirmer([](const QString &) { return true; });
    bench::Plan plan = aPlan();
    plan.algorithms = {nt, mc_thesis};
    plan.epsilons = {0.5};
    plan.repetitions = 2;
    plan.warmUp = false;
    plan.jobs = 2;
    plan.settings.clear();
    plan.outputDirectory = directory.filePath("results").toStdString();
    window.editor()->setPlan(plan);

    window.run();
    ASSERT_TRUE(window.isRunning());
    QElapsedTimer timer;
    timer.start();
    while (window.isRunning() && timer.elapsed() < 120000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(20);
    }
    ASSERT_FALSE(window.isRunning()) << "the run did not finish in two minutes";

    //2 structures (base and base+p+z; the first step does not run) x 2
    //algorithms x 2 repetitions.
    EXPECT_EQ(window.queue()->doneCount(), 8u);
    EXPECT_EQ(window.results()->rowCount(), 4);
    auto * queue = window.queue()->findChild<QTableWidget *>("queue");
    ASSERT_NE(queue, nullptr);
    for (int row = 0; row < queue->rowCount(); ++row) {
        EXPECT_EQ(queue->item(row, 6)->text(), "solved") << queue->item(row, 7)->text().toStdString();
    }
}
