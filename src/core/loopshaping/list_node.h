#ifndef QFTBX_LOOPSHAPING_LIST_NODE_H
#define QFTBX_LOOPSHAPING_LIST_NODE_H

#include <cstdint>



/**
 * @brief Base of the live-list nodes: everything OrderedList holds is one
 * of these, ordered by the index each node carries.
 *
 * The interface is deliberately thin - an index and a virtual destructor -
 * because the list owns nodes through this base and each algorithm adds
 * its own payload below it (SearchNode, McSearchNode).
 */
class ListNode {

public:
    ListNode() = default;

    ListNode(double index) {
        this->index = index;
    }

    //Nodes are deleted through this base by their owners (the live list
    //drains its leftovers on destruction).
    virtual ~ListNode() = default;

    double getIndex() const
    {
        return index;
    }

    void setIndex(const double &value)
    {
        index = value;
    }

protected:
    //Initialised: OrderedList orders by this, and the default constructor
    //left it indeterminate.
    double index = 0.0;

};

#endif // QFTBX_LOOPSHAPING_LIST_NODE_H
