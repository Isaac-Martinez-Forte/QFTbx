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

void DatosLoopShaping::setDatos(LtiSystem *controlador, QPointF rango, qreal nPuntos){
    if (introducido){
        delete controlador;
    }

    introducido = true;

    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;
}

void DatosLoopShaping::setDatos(LtiSystem *controlador){
    if (introducido){
        delete controlador;
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



