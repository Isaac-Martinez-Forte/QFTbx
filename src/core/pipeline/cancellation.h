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
 * negligible beside a node. A helper polls a token that may be null, so a
 * caller that never cancels passes nothing.
 */

#ifndef QFTBX_CANCELLATION_H
#define QFTBX_CANCELLATION_H

#include <atomic>

namespace qftbx {

/**
 * @brief A flag the interval search reads once per node, so a run that is
 * going to take forty minutes can be given up on.
 *
 * The search polls it; whoever started the search sets it, from another
 * thread. That is the whole contract, and it is deliberately the whole
 * contract: THREADS ARE NOT THIS CLASS'S BUSINESS, nor the algorithms'.
 * The facade runs the search on its worker (qftbx::BackgroundRun) and
 * raises the token from the interface; the algorithms only ever read it.
 *
 * Relaxed ordering on both sides on purpose. There is nothing to synchronise
 * WITH: the flag carries no data, only permission to stop, and it does not
 * matter whether the search notices on this node or the next one. The read
 * is a plain load with no lock and no fence, negligible beside a node.
 */
class CancellationToken
{
public:
    /// Asks the search to stop. Safe from any thread, at any time.
    void cancel() { m_cancelled.store(true, std::memory_order_relaxed); }

    /// Whether cancellation has been asked for.
    bool cancelled() const { return m_cancelled.load(std::memory_order_relaxed); }

    /// Puts the token back to its initial state, to be reused for a new run.
    void reset() { m_cancelled.store(false, std::memory_order_relaxed); }

private:
    std::atomic<bool> m_cancelled{false};
};

/**
 * @brief Polls a token that may not be there.
 *
 * The algorithms take the token as a pointer and default it to null, so a
 * caller that never wants to cancel - every test that drives an algorithm
 * directly, for one - carries on unchanged.
 */
inline bool cancellationAsked(const CancellationToken * token)
{
    return token != nullptr && token->cancelled();
}

}

#endif
