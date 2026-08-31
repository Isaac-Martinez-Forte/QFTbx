#include "src/core/loopshaping/search_node.h"

using namespace tools;

SearchNode::SearchNode(qreal index, LtiSystem * sistema, BoxFlag flags){
    this->index = index;
    this->sistema = sistema;
    this->flags = flags;
}

SearchNode::~SearchNode() {

    if (!b) {
        if (sistema != nullptr){
            if (!b2) {
                sistema->releaseOwnership();
            }
            delete sistema;
        }
    }
}

SearchNode & SearchNode::operator=(const SearchNode &c) {

    if(this != &c) {
        index = c.index;
        flags = c.flags;
        sistema = c.sistema;
    }

    return *this;
}

bool SearchNode::operator==(const SearchNode &c) const {
    return index == c.index;
}
bool SearchNode::operator!=(const SearchNode &c) const {
    return index != c.index;
}
bool SearchNode::operator<(const SearchNode &c) const {
    return index < c.index;
}
bool SearchNode::operator>(const SearchNode &c) const {
    return index > c.index;
}
bool SearchNode::operator<=(const SearchNode &c) const {
    return index <= c.index;
}
bool SearchNode::operator>=(const SearchNode &c) const {
    return index >= c.index;
}

BoxFlag SearchNode::flag() const
{
    return flags;
}

void SearchNode::setFlag(const BoxFlag &value)
{
    flags = value;
}

LtiSystem * SearchNode::system() const
{
    return sistema;
}

void SearchNode::setSystem(LtiSystem *value)
{
    sistema = value;
}

void SearchNode::releaseOwnership() {
    b = false;
}

void SearchNode::deepDeleteSystem() {
    b2 = false;
}
