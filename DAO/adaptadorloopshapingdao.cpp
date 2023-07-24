#include "adaptadorloopshapingdao.h"

AdaptadorLoopShapingDAO::AdaptadorLoopShapingDAO()
{
}

AdaptadorLoopShapingDAO::~AdaptadorLoopShapingDAO(){
}

void AdaptadorLoopShapingDAO::setDatos(std::shared_ptr<DatosLoopShaping> datos){
    this->datos = datos;
}

std::shared_ptr<DatosLoopShaping> AdaptadorLoopShapingDAO::getLoopShaping(){
    return datos;
}
