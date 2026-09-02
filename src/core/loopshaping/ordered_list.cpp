#include "src/core/loopshaping/ordered_list.h"

#include <string>

#include "src/core/exception.h"

namespace {

bool lowestFirstOrder(qreal uno, qreal dos)
{
    return uno < dos;
}

bool highestFirstOrder(qreal uno, qreal dos)
{
    return uno > dos;
}

} // namespace

OrderedList::OrderedList(bool highestFirst, std::size_t maxNodes)
    : nodes(highestFirst ? highestFirstOrder : lowestFirstOrder),
      m_maxNodes(maxNodes)
{
}

void OrderedList::insert(std::unique_ptr<ListNode> node)
{
    if (nodes.size() >= m_maxNodes) {
        throw qftbx::ComputationError(
                "The search kept " + std::to_string(m_maxNodes) + " boxes alive at once "
                "without resolving the problem. Ask for a looser epsilon accuracy, or "
                "narrow the controller search box.");
    }

    const qreal index = node->getIndex();

    nodes.insert({index, std::move(node)});

    if (nodes.size() > m_peakSize) {
        m_peakSize = nodes.size();
    }
}

ListNode * OrderedList::first()
{
    return nodes.begin()->second.get();
}

std::unique_ptr<ListNode> OrderedList::takeFirst()
{
    std::unique_ptr<ListNode> n = std::move(nodes.begin()->second);

    nodes.erase(nodes.begin());

    return n;
}


ListNode * OrderedList::last()
{
    return std::prev(nodes.end())->second.get();
}

bool OrderedList::isEmpty()
{
    return nodes.empty();
}

std::size_t OrderedList::size() const
{
    return nodes.size();
}

std::size_t OrderedList::peakSize() const
{
    return m_peakSize;
}
