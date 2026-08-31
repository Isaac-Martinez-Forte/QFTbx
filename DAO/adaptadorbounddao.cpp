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

void AdaptadorBoundDAO::setBound(BoundaryData *boundaries){
    if (introducido){
        delete bound;
    }

    introducido = true;

    bound = boundaries;
}

BoundaryData *AdaptadorBoundDAO::getBound(){
    return bound;
}
