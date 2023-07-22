#include "adaptadorloopshapingdao.h"

AdaptadorLoopShapingDAO::AdaptadorLoopShapingDAO()
{
    introducido = false;
}

AdaptadorLoopShapingDAO::~AdaptadorLoopShapingDAO(){
    if (introducido){
        delete datos;
    }
}

void AdaptadorLoopShapingDAO::setDatos(DatosLoopShaping *datos){
    if (introducido){
        delete this->datos;
    }

    introducido = true;

    this->datos = datos;
}

DatosLoopShaping * AdaptadorLoopShapingDAO::getLoopShaping(){
    return datos;
}
