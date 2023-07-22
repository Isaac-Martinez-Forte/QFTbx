#ifndef ALG_INTERSECCION_LINEAL_H
#define ALG_INTERSECCION_LINEAL_H

#include <QVector>
#include <QPoint>
#include <QPointF>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

#include "Modelo/EstructurasDatos/datosbound.h"

#define NUM_FRONTERAS_HASH 2

/**
 * @class AlgoritmoInterseccionLineal1D
 * @brief Clase que implementa el algoritmo de cálculo de la intersección de boundaries 1D.
 *
 * @author Moisés Frutos Plaza
 */

class AlgoritmoInterseccionLineal1D
{
public:
    AlgoritmoInterseccionLineal1D();
    ~AlgoritmoInterseccionLineal1D();

    void ejecutarAlgoritmo(DatosBound * boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * vectorMetadatos);

    QVector< QVector< QVector<QPointF> * > * > * getInterseccionesBoundaries();

    QVector < QVector<QPointF> * > * getInterseccionesVectores();

    QVector <bool> * getMetadatosAbierta();

    QVector <bool> * getMetadatosArriba();

private:

    QVector < QVector < QVector<QPointF> * > * > * interseccionesBoundaries;
    QVector < QVector <QPointF> * > * interseccionesVectores;

    QVector<bool> * metadatosAbierta;
    QVector<bool> * metadatosArriba;

    qint32 funcionHash(qreal x, qreal totalFase);

    void insercionManual(QVector< QVector<QPointF> * > * fronteraHash, QVector<QPointF>::iterator it, qint32 indice, QPointF punto, qreal totalFase);

    QVector< QVector< QVector<QPointF> * > * > * crearFronterasHash(QVector< QVector<QPointF> * > * &fronterasElegidas, qreal totalFase, bool abierta, bool arriba);

    QVector<QPointF> * dibujarPrimeraCapa(QVector< QVector<QPointF> * > * &fronterasElegidas, QVector< QVector< QVector<QPointF> * > * > * &fronterasHash, qreal totalFase, bool abierta1, bool abierta2);

    QVector<QPointF> * dibujarSegundaCapa(QVector< QVector<QPointF> * > * &fronterasElegidas, QVector< QVector< QVector<QPointF> * > * > * &fronterasHash, qreal totalFase, bool abierta1, bool abierta2);

    QVector<QPointF> * unirCapas(QVector<QPointF> * &capa1, QVector<QPointF> * &capa2);

    QVector< QVector<QPointF> * > * crearInterseccionHash(QVector<QPointF> * &interseccion, qreal totalFase, qint32 npuntos);

    qint32 funcionHash(qreal x, qreal totalFase, qint32 numeroFases);

    void SelectionSort(QVector<QPointF> * array);
    QVector<QPointF> * OrdenacionPuntos(QVector<QPointF> * array);


};

#endif // ALGORITMOINTERSECCIONLINEAL_H
