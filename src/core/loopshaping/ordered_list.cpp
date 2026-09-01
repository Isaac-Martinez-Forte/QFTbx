#include "src/core/loopshaping/ordered_list.h"

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

OrderedList::OrderedList(bool mayor)
    : lista(mayor ? descendente : ascendente)
{
}

void OrderedList::insert(std::unique_ptr<ListNode> elemento)
{
    const qreal index = elemento->getIndex();

    lista.insert({index, std::move(elemento)});
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
