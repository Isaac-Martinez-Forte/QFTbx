#ifndef QFTBX_LOOPSHAPING_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_SEARCH_NODE_H

#include "cinterval.hpp"
#include "Modelo/Herramientas/tools.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/list_node.h"

using namespace tools;

class SearchNode : public N {

public:

    SearchNode() {}

    SearchNode(qreal index, LtiSystem * sistema, BoxFlag flags = ambiguous);

    ~SearchNode();

    SearchNode &operator=(const SearchNode &c) ;

    bool operator==(const SearchNode &c) const;
    bool operator!=(const SearchNode &c) const;
    bool operator<(const SearchNode &c) const;
    bool operator>(const SearchNode &c) const;
    bool operator<=(const SearchNode &c) const;
    bool operator>=(const SearchNode &c) const;

    BoxFlag flag() const;

    void setFlag(const BoxFlag &value);

    LtiSystem *system() const;

    void setSystem(LtiSystem *value);

    void releaseOwnership();
    void deepDeleteSystem();
protected:

    LtiSystem * sistema;
    BoxFlag flags;

    bool b = true;
    bool b2 = true;

};

#endif // QFTBX_LOOPSHAPING_SEARCH_NODE_H
