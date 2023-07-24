#include "listaordenada.h"

ListaOrdenada::ListaOrdenada(bool mayor)
{
    lista = std::make_shared<std::list<std::shared_ptr<N>>> ();

    if (mayor) {
        comparar = &ListaOrdenada::mayor;
    } else {
        comparar = &ListaOrdenada::menor;
    }
}


ListaOrdenada::~ListaOrdenada(){

   lista->clear();
}

void ListaOrdenada::insertar(std::shared_ptr<N> elemento){
    if (lista->empty()) {
        lista->push_back(elemento);

        return;
    }

    if (elemento->getIndex() < lista->front()->getIndex()){
        lista->push_front(elemento);
        return;
    }

    bool entra = false;
    auto it = lista->begin();

    for (; it != lista->end(); ++it){
        if ((this->*comparar)(elemento->getIndex(), (*it)->getIndex())){
            entra = true;
            break;
        }
    }

    if (entra) {
        it--;
        lista->insert(it, elemento);
        return;
    }

    lista->push_back(elemento);
}

std::shared_ptr<N> ListaOrdenada::recuperarPrimero(){
    return lista->front();
}

std::shared_ptr<N> ListaOrdenada::recuperarPrimeroBorrar() {
    auto n = lista->front();

    lista->erase(lista->begin());

    return n;
}

void ListaOrdenada::borrarPrimero(){
    lista->erase(lista->begin());
}

std::shared_ptr<N> ListaOrdenada::recuperarUltimo(){
    return lista->back();
}

void ListaOrdenada::borrarUltimo(){
    lista->erase(lista->end());
}

bool ListaOrdenada::esVacia(){
    return lista->empty();
}

bool ListaOrdenada::mayor(qreal uno, qreal dos) {
    return uno > dos;
}

bool ListaOrdenada::menor(qreal uno, qreal dos) {
    return uno < dos;
}

