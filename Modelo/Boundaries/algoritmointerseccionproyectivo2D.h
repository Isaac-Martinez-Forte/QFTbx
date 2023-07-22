#ifndef ALG_PINTOR_H
#define ALG_PINTOR_H

#include <QVector>
#include <QPoint>
#include <QPointF>
#include <iostream>
#include <QStack>
#include <string.h>

#include "Modelo/EstructurasDatos/datosbound.h"

/**
 * @class AlgoritmoInterseccionProyectivo2D
 * @brief Clase que implementa los algoritmos para cálcular la intersección de boundaries en 2D.
 *
 * @author Moisés Frutos Plaza
 */

class AlgoritmoInterseccionProyectivo2D
{
public:
    AlgoritmoInterseccionProyectivo2D();
    ~AlgoritmoInterseccionProyectivo2D();

    void ejecutarAlgoritmo(DatosBound * boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * vectorMetadatos);

    QVector <bool **> * getInterseccionesBoundaries();

private:

    QVector <bool **> * interseccionesBoundaries;

    void dibujarFrontera(QVector<QPointF> * curva, bool ** &fotoIntermedia, qint32 totalFase,
                         qint32 totalMagnitud, qint32 minDB, bool zonaPermitida);

    void rellenarZona(bool ** &fotoIntermedia, qint32 x, qint32 y, qint32 totalFase, qint32 totalMagnitud,
                   bool zonaPermitida);

};

#endif // ALG_PINTOR_H
