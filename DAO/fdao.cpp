#include "fdao.h"

FDAO::FDAO()
{
}

FDAO::~FDAO(){

}

std::shared_ptr<PlantaDAO> FDAO::getPlantaDAO(){
    return std::make_shared<AdaptadorPlantaDAO>();
}

std::shared_ptr<OmegaDAO> FDAO::getOmegaDAO(){
    return std::make_shared<AdaptadorOmegaDAO>();
}

std::shared_ptr<TemplateDAO> FDAO::getTemplateDAO(){
    return std::make_shared<AdaptadorTemplateDAO>();
}

std::shared_ptr<BoundDAO> FDAO::getBoundDAO(){
    return std::make_shared<AdaptadorBoundDAO>();
}

std::shared_ptr<EspecificacionesDAO> FDAO::getEspecificacionesDAO(){
    return std::make_shared<AdaptadorEspecificacionesDAO>();
}

std::shared_ptr<ControladorDAO> FDAO::getControladorDAO(){
    return std::make_shared<AdaptadorControladorDAO>();
}


std::shared_ptr<LoopShapingDAO> FDAO::getLoopShapingDAO(){
    return std::make_shared<AdaptadorLoopShapingDAO>();
}
