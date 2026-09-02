#include "search_node.h"

using namespace tools;

SearchNode::SearchNode(qreal index, std::unique_ptr<LtiSystem> system, BoxFlag flag)
    : m_system(std::move(system))
{
    this->index = index;
    this->flags = flag;
}

BoxFlag SearchNode::flag() const
{
    return flags;
}

void SearchNode::setFlag(const BoxFlag & value)
{
    flags = value;
}

LtiSystem * SearchNode::system() const
{
    return m_system.get();
}

void SearchNode::setSystem(std::unique_ptr<LtiSystem> value)
{
    m_system = std::move(value);
}

std::unique_ptr<LtiSystem> SearchNode::releaseSystem()
{
    return std::move(m_system);
}
