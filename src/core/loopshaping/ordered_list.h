#ifndef QFTBX_LOOPSHAPING_ORDERED_LIST_H
#define QFTBX_LOOPSHAPING_ORDERED_LIST_H

#include <map>
#include <memory>
#include <type_traits>

#include <QtGlobal>

#include "src/core/loopshaping/search_node.h"

/**
 * @brief Ceiling on the number of nodes the branch and bound may keep alive
 * at once.
 *
 * A branch and bound on a problem it cannot resolve at the requested
 * accuracy grows its live list without limit. There was no ceiling, and on
 * Linux with the default heuristic overcommit (vm.overcommit_memory = 0)
 * that does NOT end in a std::bad_alloc anyone could report: malloc keeps
 * succeeding and the OOM killer takes the process down when it touches the
 * pages, so the user loses the project with no message at all. A ceiling is
 * the only mechanism that turns that into a diagnosis.
 *
 * A live node measures 528 bytes for a two-parameter controller and 1056
 * for an eight-parameter one (measured: the tree node of the list, the
 * SearchNode, the box and its parameter vector), so this ceiling is about
 * 17 to 34 GB. It is deliberately far above the millions of nodes a normal
 * hard run reaches: it is there to catch a runaway search, not to cap a
 * legitimate one.
 *
 * NOT YET VALIDATED against a genuinely hard problem. Every fixture in the
 * tree resolves early - peaks of 1 to 281 nodes, whatever the epsilon,
 * because acc90 and planta1 both terminate on a certified feasible box
 * before the accuracy matters - so this figure is reasoned from the
 * measured node size, not confirmed by a run that approaches it. That is
 * why LoopShaping reports the peak of every run next to the elapsed time:
 * the peaks of the thesis benchmarks (the ones that take tens of minutes)
 * are what to set this against.
 *
 * A caller that needs a different ceiling passes it to the OrderedList
 * constructor; nothing plumbs it to the interface yet, which belongs with
 * the deferred usability work.
 */
inline constexpr std::size_t kDefaultMaxLiveNodes = 32000000;

/**
 * @brief Priority list of live branch & bound nodes, ordered by the node
 * index (ascending by default, descending with mayor = true).
 *
 * Backed by a std::multimap: insertion and removal are O(log n) - the
 * live-node lists grow to millions of nodes - and ties keep insertion
 * order, so the exploration is deterministic. The list OWNS the nodes it
 * holds: whatever is still queued when the search ends dies with the
 * list, and takeFirst() hands one node's ownership over. The historical
 * hand-made implementation inserted every middle element one slot too
 * early, breaking the ordering that makes the first solution of the
 * branch & bound the global optimum, and crashed inserting at the front
 * of a descending list.
 */
class OrderedList
{
public:
    OrderedList(bool mayor = false, std::size_t maxNodes = kDefaultMaxLiveNodes);

    /**
     * @brief Queues one node, taking its ownership.
     *
     * Throws qftbx::ComputationError when the list already holds maxNodes:
     * see kDefaultMaxLiveNodes for why the ceiling exists.
     */
    void insert (std::unique_ptr<ListNode> elemento);

    /// Observer on the queued node; the list keeps ownership.
    ListNode * first();

    /**
     * @brief Unlinks the first node and hands its ownership over.
     *
     * Removing and obtaining used to be two calls - first() then
     * removeFirst() - and removeFirst() did NOT delete: forgetting the first()
     * grab leaked the node silently, inside a branch and bound loop that
     * visits millions of them. The algorithms all did it right; the test did
     * not, which is how it was noticed. One call that returns the ownership
     * cannot be got wrong.
     */
    std::unique_ptr<ListNode> takeFirst();

    /// takeFirst() for a list known to hold nodes of a derived type.
    template <class T>
    std::unique_ptr<T> takeFirstAs()
    {
        static_assert(std::is_base_of<ListNode, T>::value,
                      "OrderedList only holds ListNode subclasses");

        return std::unique_ptr<T>(static_cast<T *>(takeFirst().release()));
    }

    /// Observer on the last queued node; the list keeps ownership.
    ListNode * last();

    bool isEmpty ();

    /// Nodes currently queued.
    std::size_t size () const;

    /// The most nodes ever queued at once, which is what the run cost in
    /// memory and what the ceiling has to be tuned against.
    std::size_t peakSize () const;

private:

    //Ascending or descending by node index; ties keep insertion order.
    std::multimap <qreal, std::unique_ptr<ListNode>, bool(*)(qreal, qreal)> lista;

    std::size_t m_maxNodes = kDefaultMaxLiveNodes;
    std::size_t m_peakSize = 0;

};

#endif // QFTBX_LOOPSHAPING_ORDERED_LIST_H
