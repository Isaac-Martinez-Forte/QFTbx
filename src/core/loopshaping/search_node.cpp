#include "search_node.h"

using namespace tools;

SearchNode::SearchNode(qreal index, std::unique_ptr<LtiSystem> system, BoxFlag flag)
    : sistema(std::move(system))
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
    return sistema.get();
}

void SearchNode::setSystem(std::unique_ptr<LtiSystem> value)
{
    sistema = std::move(value);
}

std::unique_ptr<LtiSystem> SearchNode::releaseSystem()
{
    return std::move(sistema);
}
