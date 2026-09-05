#include "src/core/pipeline/background_run.h"

#include <except.hpp>

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
        throw InvalidInput("A background run needs something to run.");
    }

    if (running()) {
        return false;
    }

    //The previous worker is finished but not necessarily joined: a run ends
    //by clearing m_running, which does not join anything.
    if (m_worker.joinable()) {
        m_worker.join();
    }

    m_produced = false;
    m_cancelled = false;
    m_error.clear();
    m_running.store(true, std::memory_order_release);

    m_worker = std::thread([this, work = std::move(work), done = std::move(done)]() {
        //Everything is caught. The families are unrelated - the standard
        //exceptions, and the interval library's errors, which derive from
        //nothing at all - and any one of them escaping this lambda would
        //terminate the process.
        try {
            const bool produced = work();
            finish(produced, false, std::string());
        } catch (const Cancelled &) {
            finish(false, true, std::string());
        } catch (const std::exception & failure) {
            finish(false, false, std::string(failure.what()));
        } catch (const cxsc::ERROR_ALL & intervalError) {
            finish(false, false, "the interval arithmetic refused it: "
                   + intervalError.errtext());
        } catch (...) {
            //Nothing else is expected, and "nothing else is expected" is not
            //a reason to let the process die.
            finish(false, false, "the computation failed for an unknown reason");
        }

        if (done != nullptr) {
            done();
        }
    });

    return true;
}

void BackgroundRun::finish(bool produced, bool cancelled, std::string error)
{
    m_produced = produced;
    m_cancelled = cancelled;
    m_error = std::move(error);

    //Released last, so anyone who sees running() == false also sees the three
    //fields above.
    m_running.store(false, std::memory_order_release);
}

void BackgroundRun::wait()
{
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

} // namespace qftbx
