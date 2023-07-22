#ifndef ADAPTADORESPECIFICACIONESDAO_H
#define ADAPTADORESPECIFICACIONESDAO_H

#include "especificacionesdao.h"
#include "Modelo/Herramientas/tools.h"

class AdaptadorEspecificacionesDAO : public EspecificacionesDAO
{
public:
    AdaptadorEspecificacionesDAO();
    ~AdaptadorEspecificacionesDAO();

    void setEspecificaciones (QVector <tools::dBND *> * espe);

    QVector <tools::dBND *> * getEspecificaciones();

private:
    QVector <tools::dBND *> * espe = NULL;

};

#endif // ADAPTADORESPECIFICACIONESDAO_H
