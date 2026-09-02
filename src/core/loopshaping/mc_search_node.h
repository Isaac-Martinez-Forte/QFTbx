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

    McSearchNode(qreal index, std::unique_ptr<LtiSystem> system,
                 tools::BoxFlag flags = tools::ambiguous);

    void setCutsEnabled(bool recorteActivado);
    bool cutsEnabled();

    void setStage(Stage e);
    Stage stage();

    void markFrequencyFeasible(qreal pos, qreal frec);
    bool isFrequencyFeasible(qreal key) const;
    void setFeasibleFrequencies(QHash<qreal, qreal> frequencies);
    const QHash<qreal, qreal> & feasibleFrequencies() const;

protected:

    bool recorteActivado = true;
    Stage etapa = Stage::Initial;

    QHash<qreal, qreal> m_feasibleFrequencies;
};

#endif // QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
