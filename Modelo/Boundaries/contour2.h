#ifndef CONTOUR2_H
#define CONTOUR2_H


#include <QVector>
#include <QPointF>

class Contour2
{
public:
    Contour2();

    QVector <QVector <QPointF> *> * getContour (qreal nPuntosFas, qreal nPuntosMag,
                                                qreal moverMag);

    void setDatos (qreal umbralDb, QVector <QVector <qreal> *> * sabana);

#ifdef CUDA_AVAILABLE
    QVector <QVector <QPointF> *> * getContour (qreal nPuntosFas, qreal tamFas, qreal nPuntosMag, qreal tamMag,
                                                qreal moverMag);

    void setDatos (qreal umbralDb, float * sabana);
#endif


private:

    //Altura del corte en dB, ya resuelta por el llamador (spread T_U - T_L
    //para seguimiento, boundDb de la especificacion para el resto).
    qreal umbralDb;
    QVector <QVector <qreal> *> * sabana;
#ifdef CUDA_AVAILABLE
    float * sabanaCuda;
#endif

    // Get borders and Contours
    // Direction-number		Y
    //	NE-7	N-0	NW-1	|
    //	E-6	*	W-2	v
    //	SE-5	S-4	SW-3
    // X -->
    //						            N	NO	O	SO	S	SE	E	NE
    const char	coorX8Connect[8] =	{	0,	 1,	1,	1,	0,	-1,	-1,	-1	};
    const char	coorY8Connect[8] =	{  -1,	-1,	0,	1,	1,	 1,	 0,	-1	};

};

#endif // CONTOUR2_H
