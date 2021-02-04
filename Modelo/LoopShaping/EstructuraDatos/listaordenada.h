#ifndef LISTAORDENADA_H
#define LISTAORDENADA_H

#include <QList>
#include "tripleta.h"

class ListaOrdenada
{
public:
    ListaOrdenada();
    ~ListaOrdenada();

    void insertar (N *elemento);

    N * recuperarPrimero();

    void borrarPrimero();

    N * recuperarUltimo();

    void borrarUltimo();


    bool isVacia ();

private:

    QList <N *> * lista;

};

#endif // LISTAORDENADA_H
