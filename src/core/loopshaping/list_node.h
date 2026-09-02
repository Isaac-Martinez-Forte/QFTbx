#ifndef QFTBX_LOOPSHAPING_LIST_NODE_H
#define QFTBX_LOOPSHAPING_LIST_NODE_H

#include "QtCore"


class ListNode {

public:
    ListNode(){}

    ListNode(qreal index) {
        this->index = index;
    }

    //Nodes are deleted through this base by their owners (the live list
    //drains its leftovers on destruction).
    virtual ~ListNode() {}

    qreal getIndex() const
    {
        return index;
    }

    void setIndex(const qreal &value)
    {
        index = value;
    }

protected:
    //Initialised: OrderedList orders by this, and the default constructor
    //left it indeterminate.
    qreal index = 0.0;

};

#endif // QFTBX_LOOPSHAPING_LIST_NODE_H
