#ifndef DATOSLOOPSHAPING_H
#define DATOSLOOPSHAPING_H

#include <QPointF>

#include "src/core/system/lti_system.h"

//Owns the computed loop-shaping controller: replacing it frees the
//previous one, and the record frees the last one on destruction.
class DatosLoopShaping
{
public:
    DatosLoopShaping();
    DatosLoopShaping (LtiSystem * controlador, QPointF rango, qreal nPuntos);
    ~DatosLoopShaping();

    void setDatos (LtiSystem * controlador, QPointF rango, qreal nPuntos);

    void setDatos (LtiSystem * controlador);

    LtiSystem * getControlador ();

    QPointF range ();

    qreal pointCount();

private:

    LtiSystem * controlador = nullptr;
    QPointF rango;
    qreal nPuntos;

    bool introducido;
};

#endif // DATOSLOOPSHAPING_H
