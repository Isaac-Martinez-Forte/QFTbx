#include "src/core/loopshaping/mc_search_node.h"

using namespace tools;

McSearchNode::McSearchNode(double index, std::unique_ptr<LtiSystem> system, BoxFlag flags)
    : SearchNode(index, std::move(system), flags)
{
}

void McSearchNode::setCutsEnabled(bool enabled)
{
    this->enabled = enabled;
}

bool McSearchNode::cutsEnabled()
{
    return enabled;
}

void McSearchNode::setStage(Stage e)
{
    value = e;
}

Stage McSearchNode::stage()
{
    return value;
}

void McSearchNode::markFrequencyFeasible(double pos, double frec)
{
    m_feasibleFrequencies.insert(pos, frec);
}

bool McSearchNode::isFrequencyFeasible(double key) const
{
    return m_feasibleFrequencies.contains(key);
}

void McSearchNode::setFeasibleFrequencies(QHash<double, double> frequencies)
{
    m_feasibleFrequencies = std::move(frequencies);
}

const QHash<double, double> & McSearchNode::feasibleFrequencies() const
{
    return m_feasibleFrequencies;
}
