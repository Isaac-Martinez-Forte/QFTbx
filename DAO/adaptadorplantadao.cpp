#include "adaptadorplantadao.h"

AdaptadorPlantaDAO::AdaptadorPlantaDAO() : PlantaDAO()
{
}

AdaptadorPlantaDAO::~AdaptadorPlantaDAO(){
    if (this->planta != NULL)
        delete planta;
}

Sistema *AdaptadorPlantaDAO::getPlanta(){
    return planta;
}

void AdaptadorPlantaDAO::setPlanta(Sistema *planta){

    if (this->planta != NULL)
        delete this->planta;

    this->planta = planta;
}
