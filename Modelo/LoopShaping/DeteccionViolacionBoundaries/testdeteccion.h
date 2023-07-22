#ifndef TESTDETECCION_H
#define TESTDETECCION_H

#include<QVector>
#include<QPoint>
#include<QPointF>
#include<iostream>
#include <QElapsedTimer>
#include "../Herramientas/tools.h"
#include "../Boundaries/AlgoritmoInterseccionProyectivo2D.h"
#include "../Boundaries/AlgoritmoInterseccionLineal1D.h"
#include "DeteccionViolacionBoundaries1D.h"
#include "DeteccionViolacionBoundaries2D.h"
#include "../EstructurasDatos/datosbound.h"

/**
 * @class TestDeteccion
 * @brief Clase para realizar pruebas de estrés sobre los algoritmos de deteccion tanto 1D como 2D.
 *
 * @author Moisés Frutos Plaza
 */

class TestDeteccion
{

public:
    TestDeteccion();
    ~TestDeteccion();

    void ejecutarTests(DatosBound * boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * metadatos);

private:

    QVector <bool **> * interseccionesBoundaries2D;
    QVector < QVector < QVector<QPointF> * > * > * interseccionesBoundaries1D;

    void setUp(DatosBound * &boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * metadatos);
    void testMillon2D(DatosBound * &boundaries);
    void testMillon1D(DatosBound * &boundaries);

};

#endif // TESTDETECCION_H
