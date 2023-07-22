#include "listaordenada.h"

ListaOrdenada::ListaOrdenada(bool mayor)
{
    lista = new QList <N *> ();

    if (mayor) {
        comparar = &ListaOrdenada::mayor;
    } else {
        comparar = &ListaOrdenada::menor;
    }
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



    for (qint32 i = 0; i < lista->size(); i++) {

        if ((this->*comparar)(elemento->getIndex(), lista->at(i)->getIndex())){
            lista->insert(i-1, elemento);
            return;
        }

    }

    lista->append(elemento);
}

N * ListaOrdenada::recuperarPrimero(){
    return lista->first();
}

N * ListaOrdenada::recuperarPrimeroBorrar() {
    N * n = lista->first();

    lista->removeFirst();

    return n;
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

bool ListaOrdenada::esVacia(){
    return lista->isEmpty();
}

bool ListaOrdenada::mayor(qreal uno, qreal dos) {
    return uno > dos;
}

bool ListaOrdenada::menor(qreal uno, qreal dos) {
    return uno < dos;
}

