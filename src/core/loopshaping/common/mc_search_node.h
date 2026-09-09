#ifndef QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H

#include <map>
#include <memory>


#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/common/search_node.h"
#include "src/core/loopshaping/common/stages.h"

/**
 * @brief Live-list node of the MC family: a SearchNode plus the node
 * history of thesis sec. 4.4.4 - the execution stage, the cut switch and
 * the design frequencies the node is certified feasible at.
 *
 * Shared by MC of the thesis and by MC2, which carry the same history.
 *
 * The node holds its frequency map by value, so every child of a
 * bisection receives a copy for free.
 */
namespace qftbx {

class McSearchNode : public SearchNode {

public:

    McSearchNode() = default;

    McSearchNode(double index, std::unique_ptr<LtiSystem> system,
                 qftbx::BoxFlag flags = qftbx::ambiguous);

    void setCutsEnabled(bool enabled);
    bool cutsEnabled() const;

    void setStage(Stage e);
    Stage stage() const;

    void markFrequencyFeasible(double position, double frequency);
    bool isFrequencyFeasible(double key) const;
    void setFeasibleFrequencies(std::map<double, double> frequencies);
    const std::map<double, double> & feasibleFrequencies() const;

protected:

    bool enabled = true;
    Stage value = Stage::Initial;

    std::map<double, double> m_feasibleFrequencies;
};

} // namespace qftbx

#endif // QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
