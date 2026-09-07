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

/**
 * @brief The benchmark runner driven from the interface: the run on a
 * thread of its own, its events delivered on the GUI thread.
 *
 * The runner blocks while it keeps the worker processes going, so it runs
 * on a std::thread; every event it reports is copied and posted to this
 * object's thread, where the handlers the window installed are called.
 * Plain callbacks, one listener each, as everywhere else in the GUI; the
 * crossing of threads is the one thing Qt's event queue is used for.
 */
class BenchmarkRun : public QObject
{
    Q_OBJECT

public:
    explicit BenchmarkRun(QObject * parent = nullptr);
    ~BenchmarkRun() override;

    using StartedHandler = std::function<void (const bench::Case & c)>;
    using FinishedHandler = std::function<void (const bench::Case & c, const bench::Record & record)>;
    /// The run is over: how many cases failed, and the error that ended it
    /// early, empty when none did.
    using DoneHandler = std::function<void (std::size_t failures, const QString & error)>;

    void setHandlers(StartedHandler started, FinishedHandler finished, DoneHandler done);

    bool isRunning() const { return m_running.load(); }

    /// Starts the plan, whose file is at planPath (the worker processes
    /// read it). Throws qftbx::InvalidInput when a run is in progress or the
    /// worker program cannot be found.
    void start(const bench::Plan & plan, const QString & planPath, int jobs);

    /// Asks the run to stop; the running cases are killed and reported as
    /// cancelled, and the done handler is still called.
    void cancel();

    /// The benchmark tool next to the application, or on the path; empty
    /// when it is nowhere. A program set with setWorkerProgram() takes
    /// precedence: how a test names the tool of its own build tree.
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

} // namespace qftbx

#endif // QFTBX_GUI_BENCH_BENCHMARK_RUN_H
