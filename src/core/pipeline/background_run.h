/**
 * @file
 * @brief One piece of work on a worker thread, with what it threw kept.
 *
 * An exception escaping the function of a std::thread terminates the
 * process, so everything the work throws, qftbx::Exception,
 * qftbx::Cancelled or any std::exception (the interval arithmetic reports
 * its domain errors as std::domain_error), is caught at this boundary and
 * the caller asks afterwards how the last finished run went: whether the
 * work returned true, having produced a result, whether it was cancelled,
 * and the message of what it threw, empty when nothing, also with its text
 * and arguments apart for a translation. A message rather than the
 * exception, since the caller wants text and an object built on a thread
 * that is gone should not be rethrown. One run at a time: the pipeline is
 * sequential, so nothing is gained from two, and start() returns false,
 * starting nothing and leaving the run in flight untouched, while one is
 * running. The completion callback runs on the worker thread, and getting
 * back to another thread (a queued invocation, in Qt) is the caller's
 * business; it may poll running() instead. The destructor joins the
 * worker. The outcome needs no mutex: the worker writes it before
 * releasing m_running, and it may be read only after acquiring it, never
 * while a run is in flight.
 */

#ifndef QFTBX_BACKGROUND_RUN_H
#define QFTBX_BACKGROUND_RUN_H

#include "src/core/common/message.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace qftbx {

class BackgroundRun
{
public:
    using Work = std::function<bool()>;

    using Done = std::function<void()>;

    BackgroundRun() = default;

    ~BackgroundRun();

    BackgroundRun(const BackgroundRun &) = delete;
    BackgroundRun & operator=(const BackgroundRun &) = delete;

    bool start(Work work, Done done = Done());

    bool running() const { return m_running.load(std::memory_order_acquire); }

    void wait();

    bool produced() const { return m_produced; }

    bool cancelled() const { return m_cancelled; }

    const std::string & error() const { return m_error; }

    const Message & errorMessage() const { return m_errorMessage; }

private:
    void finish(bool produced, bool cancelled, const Message & error);

    std::thread m_worker;
    std::atomic<bool> m_running{false};

    bool m_produced = false;
    bool m_cancelled = false;
    std::string m_error;
    Message m_errorMessage;
};

}

#endif
