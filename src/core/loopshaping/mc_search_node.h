#ifndef QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
#define QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H

#include <QHash>

#include "Modelo/Herramientas/tools.h"
#include "src/core/system/lti_system.h"
#include "src/core/loopshaping/search_node.h"
#include "src/core/loopshaping/stages.h"

//Live-list node of algorithm MC (thesis): a SearchNode plus the node
//history of thesis sec. 4.4.4 (execution stage, cut switch and the
//design frequencies the node is certified feasible at). The node owns
//its frequency map; every child receives a copy.
class McSearchNode : public SearchNode {

public:

    McSearchNode() {}

    McSearchNode(qreal index, LtiSystem * sistema, tools::BoxFlag flags = tools::ambiguous);

    ~McSearchNode();

    void setCutsEnabled(bool recorteActivado);
    bool cutsEnabled();

    void setStage(Stage e);
    Stage stage();

    void markFrequencyFeasible(qreal pos, qreal frec);
    bool isFrequencyFeasible(qreal key);
    void setFeasibleFrequencies(QHash<qreal, qreal> * m_feasibleFrequencies);
    QHash<qreal, qreal> * feasibleFrequencies();

protected:

    bool recorteActivado = true;
    Stage etapa = Stage::Initial;

    QHash<qreal, qreal> * m_feasibleFrequencies = nullptr;
};

#endif // QFTBX_LOOPSHAPING_MC_SEARCH_NODE_H
