#include "adaptadorbounddao.h"

AdaptadorBoundDAO::AdaptadorBoundDAO()
{
}

AdaptadorBoundDAO::~AdaptadorBoundDAO(){
}

void AdaptadorBoundDAO::setBound(std::shared_ptr<DatosBound> boundaries){
    bound = boundaries;
}

std::shared_ptr<DatosBound> AdaptadorBoundDAO::getBound(){
    return bound;
}
