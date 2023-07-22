#include "datosloopshaping.h"

DatosLoopShaping::DatosLoopShaping()
{
    introducido = false;
}

DatosLoopShaping::DatosLoopShaping(Sistema *controlador, QPointF rango, qreal nPuntos){
    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;

    introducido = false;
}

void DatosLoopShaping::setDatos(Sistema *controlador, QPointF rango, qreal nPuntos){
    if (introducido){
        delete controlador;
    }

    introducido = true;

    this->controlador = controlador;
    this->rango = rango;
    this->nPuntos = nPuntos;
}

void DatosLoopShaping::setDatos(Sistema *controlador){
    if (introducido){
        delete controlador;
    }

    introducido = true;

    this->controlador = controlador;
}

Sistema * DatosLoopShaping::getControlador(){
    return controlador;
}

QPointF DatosLoopShaping::getRango(){
    return rango;
}

qreal DatosLoopShaping::getNPuntos(){
    return nPuntos;
}



