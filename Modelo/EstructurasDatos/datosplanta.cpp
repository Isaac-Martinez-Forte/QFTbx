#include "datosplanta.h"

DatosPlanta::DatosPlanta()
{
    existen = new QVector <bool> (7, false);
}

DatosPlanta::DatosPlanta(std::shared_ptr<PlantaDAO> planta, std::shared_ptr<EspecificacionesDAO> espec, std::shared_ptr<OmegaDAO> omegas,
                         std::shared_ptr<BoundDAO> boundaries, std::shared_ptr<TemplateDAO> templates){
    DatosPlanta();
    setPlanta(planta);
    setEspecificaciones(espec);
    setOmega(omegas);
    setBoundaries(boundaries);
    setTemplates(templates);
}

DatosPlanta::~DatosPlanta(){
    delete existen;
}

void DatosPlanta::setEspecificaciones(std::shared_ptr<EspecificacionesDAO> espec){
    this->espec = espec;
    existen->insert(1, true);
}

std::shared_ptr<EspecificacionesDAO> DatosPlanta::getEspecificaciones(){
    return espec;
}

void DatosPlanta::setBoundaries(std::shared_ptr<BoundDAO> boundaries){
    this->boundaries = boundaries;
    existen->insert(5,true);
}

std::shared_ptr<BoundDAO> DatosPlanta::getBoundaries(){
    return boundaries;
}


void DatosPlanta::setTemplates(std::shared_ptr<TemplateDAO> templates){
    this->templates = templates;
    existen->insert(3,true);
    if (templates->isContorno())
        existen->insert(4,true);
}

std::shared_ptr<TemplateDAO> DatosPlanta::getTemplates(){
    return templates;
}

void DatosPlanta::setPlanta(std::shared_ptr<PlantaDAO> planta){
    this->planta = planta;
    existen->insert(0,true);
}

std::shared_ptr<PlantaDAO> DatosPlanta::getPlanta(){
    return planta;
}

void DatosPlanta::setOmega(std::shared_ptr<OmegaDAO> omegas){
    this->omegas = omegas;
    existen->insert(2,true);
}

std::shared_ptr<OmegaDAO> DatosPlanta::getOmega(){
    return omegas;
}

QVector <bool> * DatosPlanta::getExisten(){
    return existen;
}

void DatosPlanta::setControlador(std::shared_ptr<ControladorDAO> contro){
    this->contro = contro;
    existen->insert(6,true);
}

std::shared_ptr<ControladorDAO> DatosPlanta::getControlador(){
    return contro;
}

void DatosPlanta::setLoopShaping(std::shared_ptr<LoopShapingDAO> loopShaping){
    this->loopShaping = loopShaping;
    existen->insert(7, true);
}

std::shared_ptr<LoopShapingDAO> DatosPlanta::getLoopShaping(){
    return loopShaping;
}
