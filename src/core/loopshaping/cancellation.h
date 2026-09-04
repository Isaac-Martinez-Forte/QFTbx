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
 * matter whether the search notices on this node or the next one. Measured
 * cost of the read (scratchpad/atomcost.cpp): 0.363 ns, which on x86-64 is a
 * plain load with no lock and no fence. Ten million nodes come to 3.6 ms in
 * total, against nodes that cost tens of microseconds each.
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

} // namespace qftbx

#endif // QFTBX_CANCELLATION_H
