#ifndef QFTBX_LOOPSHAPING_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_SEARCH_NODE_H

#include <memory>

#include "src/core/math/sequence_vectors.h"
#include "src/core/system/lti_system.h"
#include "list_node.h"

/**
 * @brief Live-list node of the interval branch & bound: a controller
 * parameter box, its objective infimum (the list index, inherited from
 * ListNode) and its feasibility flag.
 *
 * The node OWNS its box, and says so in the type: the children of a
 * bisection are always deep copies, so no two nodes ever share one. The
 * historical node held a raw pointer plus two flags (noBorrar/noBorrar2)
 * that told its destructor how much of the box to free, because the box
 * shared its parameter vectors with its parent.
 */
class SearchNode : public ListNode {

public:

    SearchNode() {}

    SearchNode(qreal index, std::unique_ptr<LtiSystem> system,
               tools::BoxFlag flag = tools::ambiguous);

    tools::BoxFlag flag() const;
    void setFlag(const tools::BoxFlag & value);

    /// Observer on the owned box: valid while the node is.
    LtiSystem * system() const;

    /// Replaces the owned box and destroys the previous one.
    void setSystem(std::unique_ptr<LtiSystem> value);

    /// Hands the box over to the caller and leaves the node without one.
    std::unique_ptr<LtiSystem> releaseSystem();

protected:

    std::unique_ptr<LtiSystem> m_system;
    tools::BoxFlag flags = tools::ambiguous;
};

#endif // QFTBX_LOOPSHAPING_SEARCH_NODE_H
