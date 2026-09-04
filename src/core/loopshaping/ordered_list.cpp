#include "src/core/loopshaping/ordered_list.h"

#include <string>

#include "src/core/exception.h"

namespace qftbx {

namespace {

bool lowestFirstOrder(double a, double b)
{
    return a < b;
}

bool highestFirstOrder(double a, double b)
{
    return a > b;
}

} // namespace

OrderedList::OrderedList(bool highestFirst, std::size_t maxNodes)
    : m_nodes(highestFirst ? highestFirstOrder : lowestFirstOrder),
      m_maxNodes(maxNodes)
{
}

void OrderedList::insert(std::unique_ptr<ListNode> node)
{
    if (m_nodes.size() >= m_maxNodes) {
        throw qftbx::ComputationError(
                "The search kept " + std::to_string(m_maxNodes) + " boxes alive at once "
                "without resolving the problem. Ask for a looser epsilon accuracy, or "
                "narrow the controller search box.");
    }

    const double index = node->getIndex();

    m_nodes.insert({index, std::move(node)});

    if (m_nodes.size() > m_peakSize) {
        m_peakSize = m_nodes.size();
    }
}

void OrderedList::requireNodes() const
{
    if (m_nodes.empty()) {
        throw qftbx::ComputationError("The search asked the live list for a node when it holds none.");
    }
}

ListNode * OrderedList::first()
{
    requireNodes();

    return m_nodes.begin()->second.get();
}

std::unique_ptr<ListNode> OrderedList::takeFirst()
{
    requireNodes();

    std::unique_ptr<ListNode> taken = std::move(m_nodes.begin()->second);

    m_nodes.erase(m_nodes.begin());

    return taken;
}


ListNode * OrderedList::last()
{
    requireNodes();

    return std::prev(m_nodes.end())->second.get();
}

bool OrderedList::isEmpty() const
{
    return m_nodes.empty();
}

std::size_t OrderedList::size() const
{
    return m_nodes.size();
}

std::size_t OrderedList::peakSize() const
{
    return m_peakSize;
}

} // namespace qftbx
