#ifndef QFTBX_LOOPSHAPING_ORDERED_LIST_H
#define QFTBX_LOOPSHAPING_ORDERED_LIST_H

#include <map>

#include <QtGlobal>

#include "src/core/loopshaping/search_node.h"

/**
 * @brief Priority list of live branch & bound nodes, ordered by the node
 * index (ascending by default, descending with mayor = true).
 *
 * Backed by a std::multimap: insertion and removal are O(log n) - the
 * live-node lists grow to millions of nodes - and ties keep insertion
 * order, so the exploration is deterministic. The list does not own the
 * nodes. The historical hand-made implementation inserted every middle
 * element one slot too early, breaking the ordering that makes the first
 * solution of the branch & bound the global optimum, and crashed
 * inserting at the front of a descending list.
 */
class OrderedList
{
public:
    OrderedList(bool mayor = false);
    ~OrderedList();

    void insert (ListNode *elemento);

    ListNode * first();

    /**
     * @brief Unlinks the first node and hands it over.
     *
     * Removing and obtaining used to be two calls - first() then
     * removeFirst() - and removeFirst() did NOT delete: forgetting the first()
     * grab leaked the node silently, inside a branch and bound loop that
     * visits millions of them. The algorithms all did it right; the test did
     * not, which is how it was noticed. One call cannot be got wrong.
     */
    ListNode * takeFirst();

    ListNode * last();

    void removeLast();

    bool isEmpty ();

private:

    //Ascending or descending by node index; ties keep insertion order.
    std::multimap <qreal, ListNode *, bool(*)(qreal, qreal)> lista;

};

#endif // QFTBX_LOOPSHAPING_ORDERED_LIST_H
