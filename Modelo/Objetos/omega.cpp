#include "omega.h"

using namespace tools;

Omega::Omega(qreal inicio, qreal final, qint32 nPuntos, QVector<qreal> *valores, tiposOmega tipo)
{
    this->inicio = inicio;
    this->final = final;
    this->nPuntos = nPuntos;
    this->valores = valores;
    this->tipo = tipo;
}

Omega::~Omega(){
    valores->clear();
}

qreal Omega::getInicio(){
    return inicio;
}

qreal Omega::getFinal(){
    return final;
}

qint32 Omega::getNPuntos(){
    return nPuntos;
}

QVector<qreal> * Omega::getValores(){
    return valores;
}

tiposOmega Omega::getTipo(){
    return tipo;
}

void Omega::setOmega(QVector<qreal> * nueva_omega){
  //  valores->clear();
    valores = nueva_omega;
}
