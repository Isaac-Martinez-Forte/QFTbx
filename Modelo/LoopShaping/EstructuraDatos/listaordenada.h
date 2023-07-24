#ifndef LISTAORDENADA_H
#define LISTAORDENADA_H

#include <QList>
#include "tripleta.h"
#include <QtGlobal>

class ListaOrdenada
{
public:
    ListaOrdenada(bool mayor = false);
    ~ListaOrdenada();

    void insertar (std::shared_ptr<N> elemento);

    std::shared_ptr<N> recuperarPrimero();
    std::shared_ptr<N> recuperarPrimeroBorrar();


    void borrarPrimero();

    std::shared_ptr<N> recuperarUltimo();

    void borrarUltimo();

    bool esVacia ();



private:

    bool menor (qreal uno, qreal dos);
    bool mayor (qreal uno, qreal dos);

    bool (ListaOrdenada::*comparar)(qreal uno, qreal dos);

    std::shared_ptr<std::list <std::shared_ptr<N>>> lista;

};

#endif // LISTAORDENADA_H
