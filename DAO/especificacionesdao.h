#ifndef ESPECIFICACIONESDAO_H
#define ESPECIFICACIONESDAO_H


#include <QVector>

#include "Modelo/Herramientas/tools.h"


class EspecificacionesDAO
{
public:
    virtual ~EspecificacionesDAO() {}


    virtual void setEspecificaciones (QVector <tools::dBND *> * espe) = 0;

    virtual QVector <tools::dBND *> * getEspecificaciones() = 0;
};



#endif // ESPECIFICACIONESDAO_H
