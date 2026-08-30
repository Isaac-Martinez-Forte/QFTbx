#include "adaptadorplantadao.h"

AdaptadorPlantaDAO::AdaptadorPlantaDAO() : PlantaDAO()
{
}

AdaptadorPlantaDAO::~AdaptadorPlantaDAO(){
    if (this->planta != NULL)
        delete planta;
}

LtiSystem *AdaptadorPlantaDAO::getPlanta(){
    return planta;
}

void AdaptadorPlantaDAO::setPlanta(LtiSystem *planta){

    if (this->planta != NULL)
        delete this->planta;

    this->planta = planta;
}
