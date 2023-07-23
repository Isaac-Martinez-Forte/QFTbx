#ifndef DATOSLOOPSHAPING_H
#define DATOSLOOPSHAPING_H

#include <QPointF>

#include "Modelo/EstructuraSistema/sistema.h"

class DatosLoopShaping
{
public:
    DatosLoopShaping();
    DatosLoopShaping (std::shared_ptr<Sistema> controlador, QPointF rango, qreal nPuntos);

    void setDatos (std::shared_ptr<Sistema> controlador, QPointF rango, qreal nPuntos);

    void setDatos (std::shared_ptr<Sistema> controlador);

    std::shared_ptr<Sistema> getControlador ();

    QPointF getRango ();

    qreal getNPuntos();

private:

    std::shared_ptr<Sistema> controlador;
    QPointF rango;
    qreal nPuntos;

};

#endif // DATOSLOOPSHAPING_H
