#ifndef QFTBX_LOOPSHAPING_ORDERED_LIST_H
#define QFTBX_LOOPSHAPING_ORDERED_LIST_H

#include <map>
#include <memory>
#include <type_traits>

#include <QtGlobal>

#include "src/core/loopshaping/search_node.h"

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
    OrderedList(bool mayor = false);

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

private:

    //Ascending or descending by node index; ties keep insertion order.
    std::multimap <qreal, std::unique_ptr<ListNode>, bool(*)(qreal, qreal)> lista;

};

#endif // QFTBX_LOOPSHAPING_ORDERED_LIST_H
