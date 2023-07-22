#include "datosfeasible.h"

DatosFeasible::DatosFeasible (qreal index, qreal posFrecuencia, QString variable, qreal porcentaje) : N(index) {
    this->posFrecuencia = posFrecuencia;
    this->name = variable;
    this->porcentaje = porcentaje;
}

qreal DatosFeasible::getPosFrecuencia () {
    return posFrecuencia;
}

void DatosFeasible::setPosFrecuencia (qreal posFrecuencia) {
    this->posFrecuencia = posFrecuencia;
}

QString  DatosFeasible::getName () {
    return name;
}

void  DatosFeasible::setName (QString name) {
    this->name = name;
}

qreal  DatosFeasible::getPorcentaje () {
    return porcentaje;
}

void  DatosFeasible::setPorcentaje (qreal porcentaje) {
    this->porcentaje = porcentaje;
}
