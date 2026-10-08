#ifndef QFTBX_LOOPSHAPING_ORDERED_LIST_H
#define QFTBX_LOOPSHAPING_ORDERED_LIST_H

#include <map>
#include <memory>
#include <type_traits>

#include "src/core/loopshaping/common/search_node.h"

/**
 * @file
 * @brief The live list of the branch and bound, and the ceiling on the
 * number of nodes it may keep at once.
 *
 * OrderedList is a priority list of live nodes ordered by the node index,
 * ascending by default and descending with highestFirst. Ties keep their
 * insertion order, so the exploration is deterministic, and the ordering
 * is what makes the first solution of the branch and bound the global one:
 * an insertion off by one slot changes the answer. The list owns the nodes
 * it holds and whatever is queued when the search ends dies with it;
 * takeFirst() unlinks the first node and hands its ownership over in one
 * call, while first() and last() only observe. Those three throw
 * qftbx::ComputationError on an empty list, and insert() throws it when
 * the list already holds its ceiling. peakSize() is the most nodes ever
 * queued at once, which LoopShaping reports next to the elapsed time and
 * which is what to set the ceiling against.
 *
 * The ceiling exists because a search that cannot resolve the requested
 * accuracy grows its list without limit, and under Linux's default
 * overcommit that ends with the OOM killer and no message, not with a
 * std::bad_alloc anyone could report. kDefaultMaxLiveNodes is deliberately
 * far above what a hard legitimate run reaches: it catches a runaway
 * search, it does not cap a legitimate one. The ceiling in effect comes
 * from the settings file (search.max-live-nodes, qftbx::Settings::Search)
 * through the algorithm that builds the list.
 */
namespace qftbx {

inline constexpr std::size_t kDefaultMaxLiveNodes = 32000000;

class OrderedList
{
public:
    OrderedList(bool highestFirst = false, std::size_t maxNodes = kDefaultMaxLiveNodes);

    void insert (std::unique_ptr<ListNode> node);

    ListNode * first();

    std::unique_ptr<ListNode> takeFirst();

    template <class T>
    std::unique_ptr<T> takeFirstAs()
    {
        static_assert(std::is_base_of<ListNode, T>::value,
                      "OrderedList only holds ListNode subclasses");

        return std::unique_ptr<T>(static_cast<T *>(takeFirst().release()));
    }

    ListNode * last();

    bool isEmpty () const;

    std::size_t size () const;

    std::size_t peakSize () const;
    std::size_t takenCount() const { return m_taken; }

private:

    std::multimap <double, std::unique_ptr<ListNode>, bool(*)(double, double)> m_nodes;

    void requireNodes() const;

    std::size_t m_maxNodes = kDefaultMaxLiveNodes;
    std::size_t m_peakSize = 0;
    std::size_t m_taken = 0;

};

}

#endif
