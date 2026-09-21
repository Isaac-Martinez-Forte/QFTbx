/**
 * @file
 * @brief Worker-thread execution with the exception caught at the boundary.
 *
 * The catch chain covers the core's own exceptions, cancellation, every
 * std::exception (the interval arithmetic reports domain errors as
 * std::domain_error) and anything else, because none of them is a reason
 * to let the process die. The outcome fields are written before the running
 * flag is released and read only after it is acquired, so that pair is
 * what publishes them and no mutex is needed. A finished worker may still
 * be unjoined, since a run ends by clearing the flag, so the next start
 * joins it first.
 */

#include "src/core/pipeline/background_run.h"

#include <exception>
#include <utility>

#include "src/core/common/exception.h"

namespace qftbx {

BackgroundRun::~BackgroundRun()
{
    wait();
}

bool BackgroundRun::start(Work work, Done done)
{
    if (work == nullptr) {
        throw InvalidInput(QFTBX_TR("Core", "A background run needs something to run."));
    }

    if (running()) {
        return false;
    }

    if (m_worker.joinable()) {
        m_worker.join();
    }

    m_produced = false;
    m_cancelled = false;
    m_error.clear();
    m_running.store(true, std::memory_order_release);

    m_worker = std::thread([this, work = std::move(work), done = std::move(done)]() {
        try {
            const bool produced = work();
            finish(produced, false, Message());
        } catch (const Cancelled &) {
            finish(false, true, Message());
        } catch (const Exception & failure) {
            finish(false, false, failure.message());
        } catch (const std::exception & failure) {
            finish(false, false, Message::plain(failure.what()));
        } catch (...) {
            finish(false, false, QFTBX_TR("Core", "the computation failed for an unknown reason"));
        }

        if (done != nullptr) {
            done();
        }
    });

    return true;
}

void BackgroundRun::finish(bool produced, bool cancelled, const Message & error)
{
    m_produced = produced;
    m_cancelled = cancelled;
    m_errorMessage = error;
    m_error = error.text().empty() ? std::string() : error.rendered();

    m_running.store(false, std::memory_order_release);
}

void BackgroundRun::wait()
{
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

}
