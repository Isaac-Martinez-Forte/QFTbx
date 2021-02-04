#ifndef N_H
#define N_H

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

#endif // N_H
