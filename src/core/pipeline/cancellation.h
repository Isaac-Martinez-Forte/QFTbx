/**
 * @file
 * @brief A flag that lets a long search be given up on.
 *
 * The search polls the token once per node; whoever started it sets it
 * from another thread. Threads are otherwise not this class's business nor
 * the algorithms': the facade runs the search on its worker and raises the
 * token from the interface, and the algorithms only read it. Both sides use
 * relaxed ordering on purpose, because the flag carries no data, only
 * permission to stop, and it does not matter whether the search notices on
 * this node or the next; the read is a plain load with no lock and no fence,
 * negligible beside a node. cancel() is safe from any thread at any time,
 * and reset() readies the token for a new run. The algorithms take the
 * token as a pointer defaulting to null, and cancellationAsked() polls one
 * that may be null, so a caller that never cancels passes nothing.
 */

#ifndef QFTBX_CANCELLATION_H
#define QFTBX_CANCELLATION_H

#include <atomic>

namespace qftbx {

class CancellationToken
{
public:
    void cancel() { m_cancelled.store(true, std::memory_order_relaxed); }

    bool cancelled() const { return m_cancelled.load(std::memory_order_relaxed); }

    void reset() { m_cancelled.store(false, std::memory_order_relaxed); }

private:
    std::atomic<bool> m_cancelled{false};
};

inline bool cancellationAsked(const CancellationToken * token)
{
    return token != nullptr && token->cancelled();
}

}

#endif
