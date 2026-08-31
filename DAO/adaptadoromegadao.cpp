#include "adaptadoromegadao.h"

AdaptadorOmegaDAO::AdaptadorOmegaDAO()
{
}

AdaptadorOmegaDAO::~AdaptadorOmegaDAO(){
    if (omega != NULL)
        delete omega;
}

QVector<qreal> * AdaptadorOmegaDAO::getFrecuencias(){
    return omega->values();
}

void AdaptadorOmegaDAO::setValues(Omega *omega){
        delete this->omega;

        this->omega = omega;
}

Omega * AdaptadorOmegaDAO::getOmega(){
    return omega;
}
