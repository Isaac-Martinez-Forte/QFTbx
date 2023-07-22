#include "datosplanta.h"

DatosPlanta::DatosPlanta()
{
    existen = new QVector <bool> (7, false);
}

DatosPlanta::DatosPlanta(PlantaDAO *planta, EspecificacionesDAO *espec, OmegaDAO *omegas, BoundDAO *boundaries, TemplateDAO *templates){
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

void DatosPlanta::setEspecificaciones(EspecificacionesDAO *espec){
    this->espec = espec;
    existen->insert(1, true);
}

EspecificacionesDAO * DatosPlanta::getEspecificaciones(){
    return espec;
}

void DatosPlanta::setBoundaries(BoundDAO *boundaries){
    this->boundaries = boundaries;
    existen->insert(5,true);
}

BoundDAO * DatosPlanta::getBoundaries(){
    return boundaries;
}


void DatosPlanta::setTemplates(TemplateDAO *templates){
    this->templates = templates;
    existen->insert(3,true);
    if (templates->isContorno())
        existen->insert(4,true);
}

TemplateDAO * DatosPlanta::getTemplates(){
    return templates;
}

void DatosPlanta::setPlanta(PlantaDAO * planta){
    this->planta = planta;
    existen->insert(0,true);
}

PlantaDAO * DatosPlanta::getPlanta(){
    return planta;
}

void DatosPlanta::setOmega(OmegaDAO * omegas){
    this->omegas = omegas;
    existen->insert(2,true);
}

OmegaDAO * DatosPlanta::getOmega(){
    return omegas;
}

QVector <bool> * DatosPlanta::getExisten(){
    return existen;
}

void DatosPlanta::setControlador(ControladorDAO *contro){
    this->contro = contro;
    existen->insert(6,true);
}

ControladorDAO * DatosPlanta::getControlador(){
    return contro;
}

void DatosPlanta::setLoopShaping(LoopShapingDAO *loopShaping){
    this->loopShaping = loopShaping;
    existen->insert(7, true);
}

LoopShapingDAO * DatosPlanta::getLoopShaping(){
    return loopShaping;
}
