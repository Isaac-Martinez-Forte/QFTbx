#include "src/core/loopshaping/mc_search_node.h"

using namespace tools;

McSearchNode::McSearchNode(qreal index, std::unique_ptr<LtiSystem> system, BoxFlag flags)
    : SearchNode(index, std::move(system), flags)
{
}

void McSearchNode::setCutsEnabled(bool recorteActivado)
{
    this->recorteActivado = recorteActivado;
}

bool McSearchNode::cutsEnabled()
{
    return recorteActivado;
}

void McSearchNode::setStage(Stage e)
{
    etapa = e;
}

Stage McSearchNode::stage()
{
    return etapa;
}

void McSearchNode::markFrequencyFeasible(qreal pos, qreal frec)
{
    m_feasibleFrequencies.insert(pos, frec);
}

bool McSearchNode::isFrequencyFeasible(qreal key) const
{
    return m_feasibleFrequencies.contains(key);
}

void McSearchNode::setFeasibleFrequencies(QHash<qreal, qreal> frequencies)
{
    m_feasibleFrequencies = std::move(frequencies);
}

const QHash<qreal, qreal> & McSearchNode::feasibleFrequencies() const
{
    return m_feasibleFrequencies;
}
