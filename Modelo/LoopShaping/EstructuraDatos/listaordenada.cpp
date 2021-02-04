#include "listaordenada.h"

ListaOrdenada::ListaOrdenada()
{
    lista = new QList <N *> ();
}


ListaOrdenada::~ListaOrdenada(){

   lista->clear();
}

void ListaOrdenada::insertar(N * elemento){

    if (lista->isEmpty()) {
        lista->append(elemento);

        return;
    }

    if (elemento->getIndex() < lista->first()->getIndex()){
        lista->insert(0, elemento);
        return;
    }

    for (int i = 0; i < lista->size(); i++) {

        if (elemento->getIndex() < lista->at(i)->getIndex()){
            lista->insert(i-1, elemento);
        }

    }

    lista->append(elemento);
}

N * ListaOrdenada::recuperarPrimero(){
    return lista->first();
}

void ListaOrdenada::borrarPrimero(){
    lista->removeFirst();
}

N * ListaOrdenada::recuperarUltimo(){
    return lista->last();
}

void ListaOrdenada::borrarUltimo(){
    lista->removeLast();
}

bool ListaOrdenada::isVacia(){
    return lista->isEmpty();
}


