#include "datosloopshaping.h"

DatosLoopShaping::DatosLoopShaping()
{
    introducido = false;
}

DatosLoopShaping::DatosLoopShaping(LtiSystem *controlador, QPointF rango, qreal nPuntos){
    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;

    introducido = false;
}

DatosLoopShaping::~DatosLoopShaping(){
    delete controlador;
}

void DatosLoopShaping::setDatos(LtiSystem *controlador, QPointF rango, qreal nPuntos){
    //The historical version deleted the INCOMING controller instead of the
    //stored one: a recomputation freed the new result and kept the dangling
    //pointer.
    if (this->controlador != controlador){
        delete this->controlador;
    }

    introducido = true;

    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;
}

void DatosLoopShaping::setDatos(LtiSystem *controlador){
    if (this->controlador != controlador){
        delete this->controlador;
    }

    introducido = true;

    this->controlador = controlador;
}

LtiSystem * DatosLoopShaping::getControlador(){
    return controlador;
}

QPointF DatosLoopShaping::range(){
    return rango;
}

qreal DatosLoopShaping::pointCount(){
    return nPuntos;
}



