#include "adaptadorespecificacionesdao.h"

AdaptadorEspecificacionesDAO::AdaptadorEspecificacionesDAO()
{
}

//El DAO es el DUEÑO de las especificaciones y de sus plantas embebidas:
//quien se las entrega (GUI, parser) le cede la propiedad. La GUI conserva
//sus originales entregando clones.
static void borrarEspecificaciones(QVector<tools::dBND *> * espe){
    if (espe == NULL){
        return;
    }
    foreach (tools::dBND * spec, *espe) {
        if (spec != NULL){
            delete spec->sistema;
            delete spec;
        }
    }
    delete espe;
}

AdaptadorEspecificacionesDAO::~AdaptadorEspecificacionesDAO(){
    borrarEspecificaciones(espe);
}

void AdaptadorEspecificacionesDAO::setEspecificaciones(QVector <tools::dBND *> * espe){
    if (this->espe == espe){
        return;
    }

    borrarEspecificaciones(this->espe);

    this->espe = espe;
}

QVector <tools::dBND *> * AdaptadorEspecificacionesDAO::getEspecificaciones(){
    return espe;
}
