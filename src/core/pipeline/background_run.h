#ifndef QFTBX_BACKGROUND_RUN_H
#define QFTBX_BACKGROUND_RUN_H

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace qftbx {

/**
 * @brief One piece of work on a worker thread, with what it threw remembered
 * instead of thrown again.
 *
 * The whole point is the catching. A computation here can throw four
 * unrelated things - qftbx::Exception, qftbx::Cancelled, std::exception and
 * the C-XSC error hierarchy - and the last two derive from neither
 * std::exception nor each other. An exception that escapes the function of an
 * std::thread does not propagate anywhere: it TERMINATES the process. So
 * everything is caught here, at the boundary, and the caller asks afterwards
 * how it went.
 *
 * One run at a time. The pipeline is sequential - every stage consumes the
 * output of the one before it - so there is nothing to gain from two at once
 * and a great deal of invalidation grief to gain from trying.
 */
class BackgroundRun
{
public:
    /// The work. Returns whether it produced a result.
    using Work = std::function<bool()>;

    /**
     * @brief Called when the work finishes, however it finished.
     *
     * IT RUNS ON THE WORKER THREAD. Whoever installs it is responsible for
     * getting back to wherever they need to be - in a Qt application that
     * means a queued invocation, which is the interface's business and not
     * this class's. A caller that would rather not deal with that can ignore
     * it and poll running() instead.
     */
    using Done = std::function<void()>;

    BackgroundRun() = default;

    /// Joins the worker: the object cannot outlive its thread.
    ~BackgroundRun();

    BackgroundRun(const BackgroundRun &) = delete;
    BackgroundRun & operator=(const BackgroundRun &) = delete;

    /**
     * @brief Starts the work.
     * @return false when a run is already in flight, in which case nothing
     *         is started and the previous run is untouched.
     */
    bool start(Work work, Done done = Done());

    /// Whether a run is in flight.
    bool running() const { return m_running.load(std::memory_order_acquire); }

    /// Waits for the run in flight, if there is one, and joins the worker.
    void wait();

    // --- how the last finished run ended ------------------------------------

    /// Whether the work returned true.
    bool produced() const { return m_produced; }

    /// Whether it ended by being cancelled.
    bool cancelled() const { return m_cancelled; }

    /**
     * @brief What it threw, or empty when it threw nothing.
     *
     * A message and not an exception: rethrowing on the caller's thread would
     * hand back an object built on a thread that is already gone, and the
     * caller wants to show text anyway.
     */
    const std::string & error() const { return m_error; }

private:
    void finish(bool produced, bool cancelled, std::string error);

    std::thread m_worker;
    std::atomic<bool> m_running{false};

    //Written by the worker before m_running is released, read by anyone after
    //acquiring it: the release/acquire pair on m_running is what publishes
    //them, so no mutex is needed for three fields nobody may read while a run
    //is in flight.
    bool m_produced = false;
    bool m_cancelled = false;
    std::string m_error;
};

} // namespace qftbx

#endif // QFTBX_BACKGROUND_RUN_H
