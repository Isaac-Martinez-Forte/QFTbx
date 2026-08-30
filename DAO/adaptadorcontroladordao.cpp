#include "adaptadorcontroladordao.h"

AdaptadorControladorDAO::AdaptadorControladorDAO()
{
}


AdaptadorControladorDAO::~AdaptadorControladorDAO(){
    if (controlador != NULL)
        delete controlador;
}

LtiSystem * AdaptadorControladorDAO::getControlador(){
    return controlador;
}

void AdaptadorControladorDAO::setControlador(LtiSystem *controlador){

    if (this->controlador != NULL)
        delete this->controlador;

    this->controlador = controlador;
}
