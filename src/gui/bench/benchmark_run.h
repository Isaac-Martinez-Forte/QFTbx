/**
 * @file
 * @brief The benchmark runner as driven from the interface, on its own
 * thread.
 *
 * Declares the object that runs a benchmark plan through the worker
 * processes without blocking the window: the runner goes on a standard
 * thread and every event it reports is posted to the GUI thread, where the
 * handlers the window installed are called. Handlers are plain callbacks,
 * one listener each; the crossing of threads is the one thing Qt's event
 * queue is used for. Also locates the worker program, next to the
 * application or on the path, with an override for tests; it is empty when
 * the program is nowhere.
 *
 * start() throws qftbx::InvalidInput when a run is in progress or the
 * worker program cannot be found; the plan's file at planPath must stay
 * there, since the worker processes read it. cancel() kills the running
 * cases, which are reported as cancelled, and the done handler is still
 * called, with the number of failed cases and the error that ended the run
 * early, empty when none did.
 */

#ifndef QFTBX_GUI_BENCH_BENCHMARK_RUN_H
#define QFTBX_GUI_BENCH_BENCHMARK_RUN_H

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>

#include <QObject>
#include <QString>

#include "src/bench/plan.h"
#include "src/bench/record.h"
#include "src/bench/runner.h"

namespace qftbx {

class BenchmarkRun : public QObject
{
    Q_OBJECT

public:
    explicit BenchmarkRun(QObject * parent = nullptr);
    ~BenchmarkRun() override;

    using StartedHandler = std::function<void (const bench::Case & c)>;
    using FinishedHandler = std::function<void (const bench::Case & c, const bench::Record & record)>;
    using DoneHandler = std::function<void (std::size_t failures, const QString & error)>;

    void setHandlers(StartedHandler started, FinishedHandler finished, DoneHandler done);

    bool isRunning() const { return m_running.load(); }

    void start(const bench::Plan & plan, const QString & planPath, int jobs);

    void cancel();

    static QString workerProgram();
    static void setWorkerProgram(const QString & program);

private:
    void join();

    bench::Runner m_runner;
    std::thread m_thread;
    std::atomic<bool> m_running{false};

    StartedHandler m_started;
    FinishedHandler m_finished;
    DoneHandler m_done;
};

}

#endif
