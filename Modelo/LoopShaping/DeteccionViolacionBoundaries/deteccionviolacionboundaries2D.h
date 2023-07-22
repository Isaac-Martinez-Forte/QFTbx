#ifndef DETEC_VIO_BND_H
#define DETEC_VIO_BND_H

#include <QPoint>
#include <limits>
#include "Modelo/Herramientas/tools.h"

#include <cinterval.h>
#include <interval.h>
#include <operadores.h>
#include <complex>
#include <cmath>

/**
 * @class DeteccionViolacionBoundaries2D
 * @brief Clase que usa los datos calculados en la intersección de boundaries 2D para detectar si unos puntos dados violan o no violan.
 *
 * @author Moisés Frutos Plaza
 */

class DeteccionViolacionBoundaries2D
{

public:

    DeteccionViolacionBoundaries2D();
    ~DeteccionViolacionBoundaries2D();

    tools::flags_box deteccionViolacion(QPointF punto, bool ** interseccion, qint32 totalFase, qint32 totalMagnitud, qint32 minDB);
    QVector<tools::flags_box> deteccionViolacionCaja(cinterval <qreal> intervalo, bool ** interseccion, qint32 totalFase, qint32 totalMagnitud,
                                                     qint32 minDB);

    QVector<qreal> * getMinimosMaximos();

private:

    QVector<qreal> * minimosMaximos;
    bool ejecutado;

};

#endif // DETEC_VIO_BND_H
