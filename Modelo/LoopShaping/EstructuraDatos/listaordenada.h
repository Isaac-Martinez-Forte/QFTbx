#ifndef LISTAORDENADA_H
#define LISTAORDENADA_H

#include <map>

#include <QtGlobal>

#include "tripleta.h"

/**
 * @brief Priority list of live branch & bound nodes, ordered by the node
 * index (ascending by default, descending with mayor = true).
 *
 * Backed by a std::multimap: insertion and removal are O(log n) - the
 * live-node lists grow to millions of nodes - and ties keep insertion
 * order, so the exploration is deterministic. The list does not own the
 * nodes. The historical hand-made implementation inserted every middle
 * element one slot too early, breaking the ordering that makes the first
 * solution of the branch & bound the global optimum, and crashed
 * inserting at the front of a descending list.
 */
class ListaOrdenada
{
public:
    ListaOrdenada(bool mayor = false);
    ~ListaOrdenada();

    void insertar (N *elemento);

    N * recuperarPrimero();
    N * recuperarPrimeroBorrar();

    void borrarPrimero();

    N * recuperarUltimo();

    void borrarUltimo();

    bool esVacia ();

private:

    //Ascending or descending by node index; ties keep insertion order.
    std::multimap <qreal, N *, bool(*)(qreal, qreal)> lista;

};

#endif // LISTAORDENADA_H
