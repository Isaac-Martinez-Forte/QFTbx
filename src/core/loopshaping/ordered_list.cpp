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

OrderedList::~OrderedList()
{
    //Nodes taken out of the list belong to the taker; whatever is still
    //queued when the search ends dies with the list (the historical
    //version leaked every live node left behind by a successful return).
    for (auto & entry : lista) {
        delete entry.second;
    }
}

void OrderedList::insert(ListNode * elemento)
{
    lista.insert({elemento->getIndex(), elemento});
}

ListNode * OrderedList::first()
{
    return lista.begin()->second;
}

ListNode * OrderedList::takeFirst()
{
    ListNode * n = lista.begin()->second;

    lista.erase(lista.begin());

    return n;
}


ListNode * OrderedList::last()
{
    return std::prev(lista.end())->second;
}

void OrderedList::removeLast()
{
    lista.erase(std::prev(lista.end()));
}

bool OrderedList::isEmpty()
{
    return lista.empty();
}
