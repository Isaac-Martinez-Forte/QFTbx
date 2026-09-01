#ifndef ADAPTADORESPECIFICACIONESDAO_H
#define ADAPTADORESPECIFICACIONESDAO_H

#include "especificacionesdao.h"
#include "Modelo/Herramientas/tools.h"

class AdaptadorEspecificacionesDAO : public EspecificacionesDAO
{
public:
    AdaptadorEspecificacionesDAO();
    ~AdaptadorEspecificacionesDAO();

    void setEspecificaciones (QVector <qftbx::SpecificationRecord *> * espe);

    QVector <qftbx::SpecificationRecord *> * getEspecificaciones();

private:
    QVector <qftbx::SpecificationRecord *> * espe = NULL;

};

#endif // ADAPTADORESPECIFICACIONESDAO_H
