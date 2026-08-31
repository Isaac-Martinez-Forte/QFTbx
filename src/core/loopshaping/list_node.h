#ifndef QFTBX_LOOPSHAPING_LIST_NODE_H
#define QFTBX_LOOPSHAPING_LIST_NODE_H

#include "QtCore"


class N {

public:
    N(){};

    N(qreal index) {
        this->index = index;
    }

    qreal getIndex() const
    {
        return index;
    }

    void setIndex(const qreal &value)
    {
        index = value;
    }

protected:
    qreal index;

};

#endif // QFTBX_LOOPSHAPING_LIST_NODE_H
