#ifndef QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H

#include <memory>

#include <QHash>

#include "src/core/math/sequence_vectors.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/stages.h"

/**
 * @brief Live-list node of algorithm MC (thesis): a SearchNode plus the
 * node history of thesis sec. 4.4.4 - the execution stage, the cut switch
 * and the design frequencies the node is certified feasible at.
 *
 * The node holds its frequency map by value, so every child of a
 * bisection receives a copy for free.
 */
class McSearchNode : public SearchNode {

public:

    McSearchNode() {}

    McSearchNode(double index, std::unique_ptr<LtiSystem> system,
                 tools::BoxFlag flags = tools::ambiguous);

    void setCutsEnabled(bool enabled);
    bool cutsEnabled();

    void setStage(Stage e);
    Stage stage();

    void markFrequencyFeasible(double pos, double frec);
    bool isFrequencyFeasible(double key) const;
    void setFeasibleFrequencies(QHash<double, double> frequencies);
    const QHash<double, double> & feasibleFrequencies() const;

protected:

    bool enabled = true;
    Stage value = Stage::Initial;

    QHash<double, double> m_feasibleFrequencies;
};

#endif // QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
