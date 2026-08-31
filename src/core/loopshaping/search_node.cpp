#include "search_node.h"

using namespace tools;

SearchNode::SearchNode(qreal index, LtiSystem * sistema, BoxFlag flag){
    this->index = index;
    this->sistema = sistema;
    this->flags = flag;
}

SearchNode::~SearchNode() {
    delete sistema;
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
    return sistema;
}

void SearchNode::setSystem(LtiSystem * value)
{
    sistema = value;
}
