#include "adaptadorespecificacionesdao.h"

AdaptadorEspecificacionesDAO::AdaptadorEspecificacionesDAO()
{
}

//El DAO es el DUEÑO de las especificaciones y de sus plantas embebidas:
//quien se las entrega (GUI, parser) le cede la propiedad. La GUI conserva
//sus originales entregando clones.
static void borrarEspecificaciones(QVector<qftbx::SpecificationRecord *> * espe){
    if (espe == NULL){
        return;
    }
    foreach (qftbx::SpecificationRecord * spec, *espe) {
        if (spec != NULL){
            delete spec->system;
            delete spec;
        }
    }
    delete espe;
}

AdaptadorEspecificacionesDAO::~AdaptadorEspecificacionesDAO(){
    borrarEspecificaciones(espe);
}

void AdaptadorEspecificacionesDAO::setEspecificaciones(QVector <qftbx::SpecificationRecord *> * espe){
    if (this->espe == espe){
        return;
    }

    borrarEspecificaciones(this->espe);

    this->espe = espe;
}

QVector <qftbx::SpecificationRecord *> * AdaptadorEspecificacionesDAO::getEspecificaciones(){
    return espe;
}
