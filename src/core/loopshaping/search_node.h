#ifndef QFTBX_LOOPSHAPING_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_SEARCH_NODE_H

#include "Modelo/Herramientas/tools.h"
#include "src/core/system/lti_system.h"
#include "list_node.h"

//Live-list node of the interval branch & bound: the controller parameter
//box, its objective infimum (the list index, inherited from ListNode) and
//its feasibility flag. THE NODE OWNS ITS SYSTEM: destroying the node
//destroys the box (children of a bisection are always deep copies).
class SearchNode : public ListNode {

public:

    SearchNode() {}

    SearchNode(qreal index, LtiSystem * sistema, tools::BoxFlag flag = tools::ambiguous);

    ~SearchNode();

    tools::BoxFlag flag() const;
    void setFlag(const tools::BoxFlag & value);

    LtiSystem * system() const;

    /// Replaces the owned system pointer; the CALLER disposes of the
    /// previous one (the cutting passes rebuild it in place).
    void setSystem(LtiSystem * value);

protected:

    LtiSystem * sistema = nullptr;
    tools::BoxFlag flags = tools::ambiguous;
};

#endif // QFTBX_LOOPSHAPING_SEARCH_NODE_H
