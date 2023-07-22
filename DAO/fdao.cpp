#include "fdao.h"

FDAO::FDAO()
{
}

FDAO::~FDAO(){

}

PlantaDAO * FDAO::getPlantaDAO(){
    return new AdaptadorPlantaDAO();
}

OmegaDAO * FDAO::getOmegaDAO(){
    return new AdaptadorOmegaDAO();
}

TemplateDAO * FDAO::getTemplateDAO(){
    return new AdaptadorTemplateDAO();
}

BoundDAO * FDAO::getBoundDAO(){
    return new AdaptadorBoundDAO();
}

EspecificacionesDAO * FDAO::getEspecificacionesDAO(){
    return new AdaptadorEspecificacionesDAO;
}

ControladorDAO* FDAO::getControladorDAO(){
    return new AdaptadorControladorDAO;
}


LoopShapingDAO * FDAO::getLoopShapingDAO(){
    return new AdaptadorLoopShapingDAO;
}
