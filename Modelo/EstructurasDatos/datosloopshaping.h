#ifndef DATOSLOOPSHAPING_H
#define DATOSLOOPSHAPING_H

#include <QPointF>

#include "Modelo/EstructuraSistema/sistema.h"

class DatosLoopShaping
{
public:
    DatosLoopShaping();
    DatosLoopShaping (Sistema * controlador, QPointF rango, qreal nPuntos);

    void setDatos (Sistema * controlador, QPointF rango, qreal nPuntos);

    void setDatos (Sistema * controlador);

    Sistema * getControlador ();

    QPointF getRango ();

    qreal getNPuntos();

private:

    Sistema * controlador;
    QPointF rango;
    qreal nPuntos;

    bool introducido;
};

#endif // DATOSLOOPSHAPING_H
