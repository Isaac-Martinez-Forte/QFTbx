#ifndef QFTBX_LOOPSHAPING_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_SEARCH_NODE_H

#include "src/core/loopshaping/loop_shaping_types.h"
#include <memory>

#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/list_node.h"

/**
 * @file
 * @brief Live-list node of the interval branch & bound: a controller
 * parameter box, its objective infimum (the list index, inherited from
 * ListNode) and its feasibility flag.
 *
 * The node OWNS its box, and says so in the type: the children of a
 * bisection are always deep copies, so no two nodes ever share one and no
 * node has to be told how much of its box it may free.
 */
namespace qftbx {

class SearchNode : public ListNode {

public:

    SearchNode() = default;

    SearchNode(double index, std::unique_ptr<LtiSystem> system,
               qftbx::BoxFlag flag = qftbx::ambiguous);

    qftbx::BoxFlag flag() const;
    void setFlag(const qftbx::BoxFlag & value);

    /// Observer on the owned box: valid while the node is.
    LtiSystem * system() const;

    /// Replaces the owned box and destroys the previous one.
    void setSystem(std::unique_ptr<LtiSystem> value);

    /// Hands the box over to the caller and leaves the node without one.
    std::unique_ptr<LtiSystem> releaseSystem();

protected:

    std::unique_ptr<LtiSystem> m_system;
    qftbx::BoxFlag flags = qftbx::ambiguous;
};

}

#endif
