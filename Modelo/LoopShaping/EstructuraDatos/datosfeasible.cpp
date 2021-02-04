#include "datosfeasible.h"

DatosFeasible::DatosFeasible (qreal index, qreal omega, QString variable, qreal porcentaje) : N(index) {
    this->omega = omega;
    this->name = variable;
    this->porcentaje = porcentaje;
}

qreal DatosFeasible::getOmega () {
    return omega;
}

void DatosFeasible::setOmega (qreal omega) {
    this->omega = omega;
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
