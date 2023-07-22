#include "adaptadorcontroladordao.h"

AdaptadorControladorDAO::AdaptadorControladorDAO()
{
}


AdaptadorControladorDAO::~AdaptadorControladorDAO(){
    if (controlador != NULL)
        delete controlador;
}

Sistema * AdaptadorControladorDAO::getControlador(){
    return controlador;
}

void AdaptadorControladorDAO::setControlador(Sistema *controlador){

    if (this->controlador != NULL)
        delete this->controlador;

    this->controlador = controlador;
}
