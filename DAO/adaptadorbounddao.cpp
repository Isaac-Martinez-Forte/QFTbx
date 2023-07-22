#include "adaptadorbounddao.h"

AdaptadorBoundDAO::AdaptadorBoundDAO()
{
    introducido = false;
}

AdaptadorBoundDAO::~AdaptadorBoundDAO(){
    if (introducido){
        delete bound;
    }
}

void AdaptadorBoundDAO::setBound(DatosBound *boundaries){
    if (introducido){
        delete bound;
    }

    introducido = true;

    bound = boundaries;
}

DatosBound *AdaptadorBoundDAO::getBound(){
    return bound;
}
