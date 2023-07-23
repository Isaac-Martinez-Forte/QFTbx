#include "adaptadorplantadao.h"

AdaptadorPlantaDAO::AdaptadorPlantaDAO() : PlantaDAO()
{
}

AdaptadorPlantaDAO::~AdaptadorPlantaDAO(){
}

std::shared_ptr<Sistema> AdaptadorPlantaDAO::getPlanta(){
    return planta;
}

void AdaptadorPlantaDAO::setPlanta(std::shared_ptr<Sistema> planta){
    this->planta = planta;
}
