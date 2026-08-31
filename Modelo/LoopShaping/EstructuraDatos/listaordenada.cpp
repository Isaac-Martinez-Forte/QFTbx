#include "listaordenada.h"

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

ListaOrdenada::ListaOrdenada(bool mayor)
    : lista(mayor ? descendente : ascendente)
{
}

ListaOrdenada::~ListaOrdenada()
{
    //The list does not own the nodes: the algorithms keep and delete them.
}

void ListaOrdenada::insertar(N * elemento)
{
    lista.insert({elemento->getIndex(), elemento});
}

N * ListaOrdenada::recuperarPrimero()
{
    return lista.begin()->second;
}

N * ListaOrdenada::recuperarPrimeroBorrar()
{
    N * n = lista.begin()->second;

    lista.erase(lista.begin());

    return n;
}

void ListaOrdenada::borrarPrimero()
{
    lista.erase(lista.begin());
}

N * ListaOrdenada::recuperarUltimo()
{
    return std::prev(lista.end())->second;
}

void ListaOrdenada::borrarUltimo()
{
    lista.erase(std::prev(lista.end()));
}

bool ListaOrdenada::esVacia()
{
    return lista.empty();
}
