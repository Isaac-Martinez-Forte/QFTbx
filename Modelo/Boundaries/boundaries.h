#ifndef BOUNDARIES_H
#define BOUNDARIES_H

#include <QVector>
#include <complex>
#include <qmath.h>
#include <QTransform>
#include <limits>
#include <qprogressbar.h>
#include <cmath>
#ifdef OpenMP_AVAILABLE
#include <omp.h>
#endif
#include <QMap>
#include <QHash>

#include "../EstructuraSistema/sistema.h"
#include "../Herramientas/tools.h"
#include "contour2.h"
#include "contour.h"
#include "../EstructurasDatos/var.h"
#include "../EstructurasDatos/datosbound.h"
#include "algoritmointerseccionlineal1D.h"

/**
 * @class Boundaries
 * @brief Clase que implementa los algoritmos de cálculo de boundaries.
 *
 * @author Isaac Martínez Forte
 */

class Boundaries : public QObject
{
public:

    /**
    * @fn Boundaries
    * @brief Constructor de la clase.
   */

    Boundaries();


    /**
    * @fn lanzarCalculo
    * @brief Lanza el cálculo de los boundaries.
    *
    * @param omega vector de frecuencias que se utiliza para calcular los boundaries
    * @param planta de la cual queremos calcular sus boundaries
    * @param templates de la planta previamente calculados.
    * @param altura número real que se utiliza para "cortar" a la altura determinada la sábana generada por el algoritmo.
    * @param datosFas números de inicio y de fin del eje de coordenadas de las fases.
    * @param puntosFas número de puntos que tiene el eje de coordenadas correspondiente a las fases.
    * @param datosMag números de inicio y de fin del eje de coordenadas de las magnitudes.
    * @param puntosMag número de puntos que tiene el eje de coordenadas correspondiente a las magnitudes.
    * @param infinito número real que representa el infinito, si es 0 se toma como vacío.
   */

    void lanzarCalculo(QVector <qreal> * omega, Sistema * planta, QVector<QVector<std::complex <qreal> > *> *templates,
                       QVector <tools::dBND *> * altura, QPointF datosFas,
                       qint32 puntosFas, QPointF datosMag, qint32 puntosMag, qreal infinito, bool cuda);


    /**
    * @fn bndEstabilidad
    * @brief Calcula la sábana del boundaries de estabilidad para la planta y las frecuencias de diseño introducidas a partir de los templates calculados.
    *
    * @param omega vector de frecuencias que se utiliza para calcular los boundaries
    * @param planta de la cual queremos calcular sus boundaries
    * @param templates de la planta previamente calculados.
    * @param datosFas números de inicio y de fin del eje de coordenadas de las fases.
    * @param puntosFas número de puntos que tiene el eje de coordenadas correspondiente a las fases.
    * @param datosMag números de inicio y de fin del eje de coordenadas de las magnitudes.
    * @param puntosMag número de puntos que tiene el eje de coordenadas correspondiente a las magnitudes.
    * @param infinito número real que representa el infinito, si es 0 se toma como vacío.
    *
    * @return sábana con el cálculo intermedio del boundarie de estabilidad.
   */

    void bnd (QVector <qreal> * omega, Sistema * planta, QVector<QVector<std::complex <qreal> > *> *templates, QPointF datosFas,
              qint32 puntosFas, QPointF datosMag, qint32 puntosMag, qreal infinito);


    /**
    * @fn calcularContour
    * @brief Función que da un "corte" a una sábana calculada previamente y con ello calcula el boundarie correspondiente.
    *
    * @param altura número real que se utiliza para "cortar" a la altura determinada la sábana generada por el algoritmo.
    * @param datosFas números de inicio y de fin del eje de coordenadas de las fases.
    * @param datosMag números de inicio y de fin del eje de coordenadas de las magnitudes.
    * @param sabana a la cual hay que darle el "corte" a la altura especificada.
   */

    void calcularContour(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound, QVector<QVector<QVector<qreal> *> *> *vecSabanas,
                         QMap<QString, QVector<QPoint> *> *metaBound, std::complex<qreal> p0, QVector<std::complex<qreal> > *p,
                         qint32 contador, qreal nPuntosFas, qreal nPuntosMag, qreal moverMag);


    void calcularContour(qreal omega, QMap <QString, QVector <QVector <QPointF> * > *> * bound,
                                     std::vector <float *> * vecSabanasCuda,
                                     QMap <QString, QVector <QPoint> * > * metaBound,
                                     std::complex <qreal> p0, QVector <std::complex <qreal> > * p, qint32 contador,
                                     qreal nPuntosFas, qreal nPuntosMag, qreal moverMag);


    /**
    * @fn getBoundaries
    * @brief Función que retorna los boundaries calculados.
    *
    * @return Boundaries calculados.
   */

    DatosBound* getBoundaries();

    QVector <QMap <QString, QVector <QPoint> * > *> * getMetaDatosBoundaries();

    QVector<QVector<QPointF> * > *getBoundariesReunidos();

    QVector <qreal> * getOmega();


private:
    bool pnpoly(QVector <QPointF * > * vector, QPointF * punto);

    bool repetido;

    bool boundariesCreados;

    QVector<tools::dBND *> * altura;

    qreal unwrap(qreal previousAngle,qreal newAngle);
    qreal constrainAngle(qreal x);
    qreal angleConv(qreal angle);
    qreal angleDiff(qreal a,qreal b);

    QVector<QVector<QPointF> *> *calcularContourVector(tools::dBND * altura, QVector<QVector<qreal> *> *sabana, qreal omega,
                                                      QVector<QPoint> *metaBoun, std::complex<qreal> p0, QVector<std::complex<qreal> > *p,
                                                      qint32 i, qreal nPuntosFas, qreal nPuntosMag,
                                                       qreal moverMag, tools::dBND *altura2 = NULL);

    QVector<QVector<QPointF> *> *calcularContourVector(tools::dBND * altura, float * sabana, qreal omega,
                                                      QVector<QPoint> *metaBoun, std::complex<qreal> p0, QVector<std::complex<qreal> > *p,
                                                      qint32 i, qreal nPuntosFas, qreal nPuntosMag,
                                                       qreal moverMag, tools::dBND *altura2 = NULL);

    void calcularBndOmega(qreal omega, Sistema * planta,
                                       QVector<std::complex <qreal> > * temp, QVector <qreal> * fases,
                                       QVector <qreal> * mag, qreal inf, qint32 contador, qint32 num_hilo,
                          QVector<qreal> *nueva_omega);


    QPointF datosFas;
    QPointF datosMag;
    qint32 tamFas;
    qint32 tamMag;


    QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries;
    QVector <QMap <QString, QVector <QPoint> * > *> * metaDatosBoundaries;
    QVector <QVector <QPointF> * > * boun_reunidos;
    QVector< QVector< QVector<QPointF> * > * > * boundariesHash;


    qint32 getZona(QVector <QPointF> * vec, std::complex<qreal> p0, QVector<std::complex<qreal> > *p, qint32 i, qreal altura);

    QVector <bool> * boolseguimiento;
    QVector <bool> * boolestabilidad;
    QVector <bool> * boolruido;
    QVector <bool> * boolRPS;
    QVector <bool> * boolRPE;
    QVector <bool> * boolEC;
    bool boolcreados;

    QVector <bool> * metaDatosAbierta;
    QVector <bool> * metaDatosArriba;

    QVector <qreal> * nueva_omega;

    bool cuda;

};

#endif // BOUNDARIES_H
