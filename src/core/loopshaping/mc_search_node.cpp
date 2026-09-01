#include "src/core/loopshaping/mc_search_node.h"

using namespace tools;

McSearchNode::McSearchNode(qreal index, LtiSystem * sistema, BoxFlag flags)
    : SearchNode(index, sistema, flags)
{
}

McSearchNode::~McSearchNode()
{
    delete m_feasibleFrequencies;
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
    if (m_feasibleFrequencies == nullptr) {
        m_feasibleFrequencies = new QHash<qreal, qreal>();
    }

    m_feasibleFrequencies->insert(pos, frec);
}

bool McSearchNode::isFrequencyFeasible(qreal key)
{
    return m_feasibleFrequencies != nullptr && m_feasibleFrequencies->contains(key);
}

void McSearchNode::setFeasibleFrequencies(QHash<qreal, qreal> * frequencies)
{
    delete m_feasibleFrequencies;
    m_feasibleFrequencies = frequencies;
}

QHash<qreal, qreal> * McSearchNode::feasibleFrequencies()
{
    return m_feasibleFrequencies;
}
