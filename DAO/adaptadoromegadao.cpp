#include "adaptadoromegadao.h"

AdaptadorOmegaDAO::AdaptadorOmegaDAO()
{
}

AdaptadorOmegaDAO::~AdaptadorOmegaDAO(){
    if (omega != NULL)
        delete omega;
}

QVector<qreal> * AdaptadorOmegaDAO::getFrecuencias(){
    return omega->getValores();
}

void AdaptadorOmegaDAO::setOmega(Omega *omega){
        this->omega = omega;
}

Omega * AdaptadorOmegaDAO::getOmega(){
    return omega;
}
