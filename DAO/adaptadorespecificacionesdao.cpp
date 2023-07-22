#include "adaptadorespecificacionesdao.h"

AdaptadorEspecificacionesDAO::AdaptadorEspecificacionesDAO()
{
}

AdaptadorEspecificacionesDAO::~AdaptadorEspecificacionesDAO(){
    if (espe != NULL){
        espe->clear();
    }
}

void AdaptadorEspecificacionesDAO::setEspecificaciones(QVector <tools::dBND *> * espe){
    if (this->espe != NULL){
        this->espe->clear();
    }

    this->espe = espe;
}

QVector <tools::dBND *> * AdaptadorEspecificacionesDAO::getEspecificaciones(){
    return espe;
}
