#include "adaptadorcontroladordao.h"

AdaptadorControladorDAO::AdaptadorControladorDAO()
{
}


AdaptadorControladorDAO::~AdaptadorControladorDAO(){
}

std::shared_ptr<Sistema> AdaptadorControladorDAO::getControlador(){
    return controlador;
}

void AdaptadorControladorDAO::setControlador(std::shared_ptr<Sistema> controlador){
    this->controlador = controlador;
}
