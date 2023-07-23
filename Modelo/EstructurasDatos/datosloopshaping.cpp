#include "datosloopshaping.h"

DatosLoopShaping::DatosLoopShaping()
{
}

DatosLoopShaping::DatosLoopShaping(std::shared_ptr<Sistema> controlador, QPointF rango, qreal nPuntos){
    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;

}

void DatosLoopShaping::setDatos(std::shared_ptr<Sistema> controlador, QPointF rango, qreal nPuntos){

    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;
}

void DatosLoopShaping::setDatos(std::shared_ptr<Sistema> controlador){

    this->controlador = controlador;
}

std::shared_ptr<Sistema> DatosLoopShaping::getControlador(){
    return controlador;
}

QPointF DatosLoopShaping::getRango(){
    return rango;
}

qreal DatosLoopShaping::getNPuntos(){
    return nPuntos;
}



