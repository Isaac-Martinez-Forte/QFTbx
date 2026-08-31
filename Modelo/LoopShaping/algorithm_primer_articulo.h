#ifndef ALGORITHM_PRIMER_ARTICULO_H
#define ALGORITHM_PRIMER_ARTICULO_H

#include <QVector>
#include <QHash>
#include <cmath>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <limits>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "NaturalIntervalExtension/natural_interval_extension.h"
#include "EstructuraDatos/avl.h"
#include "EstructuraDatos/tripleta.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "DeteccionViolacionBoundaries/deteccionviolacionboundaries.h"
#include "EstructuraDatos/listaordenada.h"
#include "funcionescomunes.h"
#include "GUI/boundary_union_viewer.h"
#include "interval.hpp"
#include "Modelo/LoopShaping/EstructuraDatos/data_box.h"

using namespace cxsc;

class Algorithm_primer_articulo : public QObject
{
    Q_OBJECT

public:
    Algorithm_primer_articulo();
    ~Algorithm_primer_articulo();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, BoundaryData * boundaries,
                    qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash, bool depuracion,
                   bool hilos, QVector <qreal> * radiosBoundariesMayor, QVector <qreal> * radiosBoundariesMenor,
                   QVector <QPointF> * centros, bool biseccion_avanzada, bool deteccion_avanzada, bool a);

    bool init_algorithm();

    LtiSystem * getControlador();

private:

    inline Tripleta *check_box_feasibility(LtiSystem *controlador);
    inline LtiSystem *aceleratedNuevo(LtiSystem *t, QVector<data_box *> *datosCortesBoundaries);
    inline LtiSystem *aceleratedAntiguo(LtiSystem *t, QVector<data_box *> *datosCortesBoundaries);



    inline void comprobarVariables ( LtiSystem * controlador);
    inline FC::return_bisection split_box_bisection_avanced(LtiSystem * current_controlador);
    inline FC::return_bisection split_box_bisection(LtiSystem * current_controlador);


    LtiSystem * planta;
    LtiSystem * controlador;
    QVector <qreal> * omega;
    BoundaryData * boundaries;
    BoundaryData * boundariesAux;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;

    LtiSystem * controlador_retorno;
    qreal minimo_boundaries;

    QPointF interseccion (QPointF uno, QPointF dos);

    qint32 tamFas;

    bool depuracion;

    QMutex mutexAccesoLista;
    QMutex mutexAccesoContador;
    QMutex mutexEnding;
    QMutex mutexTerminar;
    qint32 contadorHilos = 0;
    bool hilos;
    QSemaphore * semaforo;

    QVector <qreal> * radiosBoundariesMayor;
    QVector <qreal> * radiosBoundariesMenor;
    QVector <QPointF> * centros;
    bool terminacionCorrecta;


    bool isVariableNume;
    bool isVariableDeno;
    
    FC::return_bisection (Algorithm_primer_articulo::*split_box)(LtiSystem *);
    data_box * (DeteccionViolacionBoundaries::*deteccionViolacion) (cinterval, BoundaryData *, qint32);

    LtiSystem * (Algorithm_primer_articulo::*analisis)(LtiSystem *v, QVector<data_box *> *datosCortesBoundaries);

    bool Nyquist;


    DeteccionViolacionBoundaries * deteccion;
    QVector <cxsc::complex> * plantas_nominales;
    QVector <std::complex <qreal>> * plantas_nominales2;


    QVector <QVector<qreal> *> * f;
};

#endif

