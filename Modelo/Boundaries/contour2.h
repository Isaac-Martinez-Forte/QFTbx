#ifndef CONTOUR2_H
#define CONTOUR2_H


#include <QVector>
#include <QPointF>
#include "Modelo/Herramientas/tools.h"

class Contour2
{
public:
    Contour2();

    QVector <QVector <QPointF> *> * getContour (qreal nPuntosFas, qreal nPuntosMag,
                                                qreal moverMag);

    QVector <QVector <QPointF> *> * getContour (qreal nPuntosFas, qreal tamFas, qreal nPuntosMag, qreal tamMag,
                                                qreal moverMag);

    void setDatos (tools::dBND * altura, QVector <QVector <qreal> *> * sabana, qreal omega,
                   tools::dBND * altura2 = NULL);

    void setDatos (tools::dBND * altura, float * sabana, qreal omega,
                   tools::dBND * altura2 = NULL);


private:

    tools::dBND * altura;
    tools::dBND * altura2;
    QVector <QVector <qreal> *> * sabana;
    float * sabanaCuda;
    bool ** sabana_bool;
    qreal omega;

    qreal getAltura();

    // Get borders and Contours
    // Direction-number		Y
    //	NE-7	N-0	NW-1	|
    //	E-6	*	W-2	v
    //	SE-5	S-4	SW-3
    // X -->
    //						            N	NO	O	SO	S	SE	E	NE
    const char	coorX8Connect[8] =	{	0,	 1,	1,	1,	0,	-1,	-1,	-1	};
    const char	coorY8Connect[8] =	{  -1,	-1,	0,	1,	1,	 1,	 0,	-1	};

    QVector <bool> * cerrados;

    QVector <bool> * mascara;


};

#endif // CONTOUR2_H
