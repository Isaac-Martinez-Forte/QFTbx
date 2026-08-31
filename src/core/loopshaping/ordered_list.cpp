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
    //The list does not own the nodes: the algorithms keep and delete them.
}

void OrderedList::insert(N * elemento)
{
    lista.insert({elemento->getIndex(), elemento});
}

N * OrderedList::first()
{
    return lista.begin()->second;
}

N * OrderedList::takeFirst()
{
    N * n = lista.begin()->second;

    lista.erase(lista.begin());

    return n;
}

void OrderedList::removeFirst()
{
    lista.erase(lista.begin());
}

N * OrderedList::last()
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
