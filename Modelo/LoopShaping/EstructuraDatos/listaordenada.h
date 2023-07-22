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

    void insertar (N *elemento);

    N * recuperarPrimero();
    N * recuperarPrimeroBorrar();


    void borrarPrimero();

    N * recuperarUltimo();

    void borrarUltimo();

    bool esVacia ();



private:

    bool menor (qreal uno, qreal dos);
    bool mayor (qreal uno, qreal dos);

    bool (ListaOrdenada::*comparar)(qreal uno, qreal dos);

    QList <N *> * lista;

};

#endif // LISTAORDENADA_H
