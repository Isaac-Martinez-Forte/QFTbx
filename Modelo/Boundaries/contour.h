#ifndef CONTOUR_H
#define CONTOUR_H

#include <QVector>
#include <QPointF>
#include "Modelo/Herramientas/tools.h"

class Contour
{
public:
    Contour();
    QVector <QVector <QPointF> *> * getContour (qreal nPuntosFas, qreal tamFas, qreal nPuntosMag, qreal tamMag,
                                                qreal moverMag);

    void setDatos (tools::dBND * altura, QVector <QVector <qreal> *> * sabana, qreal omega);

private:

    tools::dBND * altura;
    tools::dBND * altura2;
    QVector <QVector <qreal> *> * sabana;
    bool ** sabana_bool;
    qreal omega;

    QVector<QPointF> * getContourFromBorderPoint(QVector <QVector <qreal> *> * sabana, qint32 posx, qint32 posy, qreal altura,
                                                         QVector <QPointF> * lista_aux, qreal nPuntosFas,
                                                 qreal tamFas, qreal nPuntosMag, qreal tamMag, qreal moverMag);
    //QVector<QPointF> * getContourFromBorderPoint(qint32 posx, qint32 posy, qreal alto, qreal ancho,
    //                                                     QVector <QPointF> * lista_aux);

    QVector <QPointF> line(qint32 x1, qint32 y1, qint32 x2, qint32 y2);

    qreal getAltura();

    // Get borders and Contours
    // Direction-number        Y
    //    NE-7    N-0    NW-1    |
    //    E-6    *    W-2    v
    //    SE-5    S-4    SW-3
    // X -->

    //                                     N    NO    O    SO    S    SE    E    NE
    const char    coorX8Connect[8] =    {    -1,    -1,    0,    1,    1,     1,     0,    -1    };
    const char    coorY8Connect[8] =    {     0,     1,    1,    1,    0,    -1,    -1,    -1    };


    QVector <bool> * cerrados;
};

#endif // CONTOUR_H
