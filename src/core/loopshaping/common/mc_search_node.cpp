/**
 * @file
 * @brief The state a node of the MC family carries beyond the shared node.
 */

#include <utility>

#include "src/core/loopshaping/common/mc_search_node.h"

namespace qftbx {

McSearchNode::McSearchNode(double index, std::unique_ptr<LtiSystem> system, BoxFlag flags)
    : SearchNode(index, std::move(system), flags)
{
}

void McSearchNode::setCutsEnabled(bool enabled)
{
    m_cutsEnabled = enabled;
}

bool McSearchNode::cutsEnabled() const
{
    return m_cutsEnabled;
}

void McSearchNode::setStage(Stage e)
{
    m_stage = e;
}

Stage McSearchNode::stage() const
{
    return m_stage;
}

void McSearchNode::markFrequencyFeasible(std::size_t frequency)
{
    if (frequency >= m_feasibleAt.size()) {
        m_feasibleAt.resize(frequency + 1, 0);
    }
    m_feasibleAt[frequency] = 1;
}

bool McSearchNode::isFrequencyFeasible(std::size_t frequency) const
{
    return frequency < m_feasibleAt.size() && m_feasibleAt[frequency] != 0;
}

void McSearchNode::inheritHistoryFrom(const McSearchNode & parent)
{
    m_cutsEnabled = parent.m_cutsEnabled;
    m_stage = parent.m_stage;
    m_feasibleAt = parent.m_feasibleAt;
}

void McSearchNode::setCornerVerdict(bool certified)
{
    m_cornerVerdict = certified;
}

std::optional<bool> McSearchNode::cornerVerdict() const
{
    return m_cornerVerdict;
}

}
