#include "src/core/loopshaping/ordered_list.h"

#include <string>

#include "src/core/exception.h"

namespace {

bool ascendente(qreal uno, qreal dos)
{
    return uno < dos;
}

bool descendente(qreal uno, qreal dos)
{
    return uno > dos;
}

} // namespace

OrderedList::OrderedList(bool mayor, std::size_t maxNodes)
    : lista(mayor ? descendente : ascendente),
      m_maxNodes(maxNodes)
{
}

void OrderedList::insert(std::unique_ptr<ListNode> elemento)
{
    if (lista.size() >= m_maxNodes) {
        throw qftbx::ComputationError(
                "The search kept " + std::to_string(m_maxNodes) + " boxes alive at once "
                "without resolving the problem. Ask for a looser epsilon accuracy, or "
                "narrow the controller search box.");
    }

    const qreal index = elemento->getIndex();

    lista.insert({index, std::move(elemento)});

    if (lista.size() > m_peakSize) {
        m_peakSize = lista.size();
    }
}

ListNode * OrderedList::first()
{
    return lista.begin()->second.get();
}

std::unique_ptr<ListNode> OrderedList::takeFirst()
{
    std::unique_ptr<ListNode> n = std::move(lista.begin()->second);

    lista.erase(lista.begin());

    return n;
}


ListNode * OrderedList::last()
{
    return std::prev(lista.end())->second.get();
}

bool OrderedList::isEmpty()
{
    return lista.empty();
}

std::size_t OrderedList::size() const
{
    return lista.size();
}

std::size_t OrderedList::peakSize() const
{
    return m_peakSize;
}
