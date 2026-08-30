#ifndef DATOSLOOPSHAPING_H
#define DATOSLOOPSHAPING_H

#include <QPointF>

#include "src/core/system/lti_system.h"

class DatosLoopShaping
{
public:
    DatosLoopShaping();
    DatosLoopShaping (LtiSystem * controlador, QPointF rango, qreal nPuntos);

    void setDatos (LtiSystem * controlador, QPointF rango, qreal nPuntos);

    void setDatos (LtiSystem * controlador);

    LtiSystem * getControlador ();

    QPointF range ();

    qreal getNPuntos();

private:

    LtiSystem * controlador;
    QPointF rango;
    qreal nPuntos;

    bool introducido;
};

#endif // DATOSLOOPSHAPING_H
