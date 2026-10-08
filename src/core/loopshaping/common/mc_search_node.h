#ifndef QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/loopshaping/common/stages.h"

/**
 * @file
 * @brief Live-list node of the MC family: a SearchNode plus the node
 * history of thesis sec. 4.4.4 - the execution stage, the cut switch and
 * the design frequencies the node is certified feasible at - and, for a
 * feasible slab, the verdict its corner received when the slab was made,
 * so that the corner is not asked again when the node is taken.
 *
 * Shared by MC of the thesis, which uses the whole history, and by MC2,
 * which has no stages and alone asks for the corner verdict. The feasible
 * frequencies are kept by their index, and a child of a bisection inherits
 * the history of its parent whole. McBisectionResult holds the two
 * children of a bisection for whoever receives them, to be inserted in the
 * live list or dropped.
 */
namespace qftbx {

class McSearchNode : public SearchNode {

public:

    McSearchNode(double index, std::unique_ptr<LtiSystem> system,
                 qftbx::BoxFlag flags = qftbx::ambiguous);

    void setCutsEnabled(bool enabled);
    bool cutsEnabled() const;

    void setStage(Stage e);
    Stage stage() const;

    void markFrequencyFeasible(std::size_t frequency);
    bool isFrequencyFeasible(std::size_t frequency) const;

    void inheritHistoryFrom(const McSearchNode & parent);

    void setCornerVerdict(bool certified);
    std::optional<bool> cornerVerdict() const;

private:

    bool m_cutsEnabled = true;
    Stage m_stage = Stage::Initial;
    std::vector<char> m_feasibleAt;
    std::optional<bool> m_cornerVerdict;
};

struct McBisectionResult {
    std::unique_ptr<McSearchNode> t1;
    std::unique_ptr<McSearchNode> t2;
};

}

#endif
