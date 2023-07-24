#ifndef ALGORITHM_PRIMER_ARTICULO_H
#define ALGORITHM_PRIMER_ARTICULO_H

#include <QVector>
#include <QHash>
#include <cmath>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <limits>

#include "Modelo/EstructurasDatos/datosbound.h"
#include "Modelo/EstructuraSistema/sistema.h"
#include "NaturalIntervalExtension/natural_interval_extension.h"
#include "EstructuraDatos/avl.h"
#include "EstructuraDatos/tripleta.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructuraSistema/cpolinomios.h"
#include "Modelo/EstructuraSistema/kganancia.h"
#include "Modelo/EstructuraSistema/knganancia.h"
#include "DeteccionViolacionBoundaries/deteccionviolacionboundaries.h"
#include "EstructuraDatos/listaordenada.h"
#include "funcionescomunes.h"
#include "GUI/viewboundreun.h"
#include "interval.hpp"
#include "Modelo/LoopShaping/EstructuraDatos/data_box.h"

using namespace cxsc;

class Algorithm_primer_articulo : public QObject
{
    Q_OBJECT

public:
    Algorithm_primer_articulo();
    ~Algorithm_primer_articulo();

    void set_datos(std::shared_ptr<Sistema> planta, std::shared_ptr<Sistema> controlador, QVector<qreal> *omega, std::shared_ptr<DatosBound> boundaries,
                    qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash, bool depuracion,
                   bool hilos, QVector <qreal> * radiosBoundariesMayor, QVector <qreal> * radiosBoundariesMenor,
                   QVector <QPointF> * centros, bool biseccion_avanzada, bool deteccion_avanzada, bool a);

    bool init_algorithm();

    std::shared_ptr<Sistema> getControlador();

private:

    inline std::shared_ptr<Tripleta> check_box_feasibility(std::shared_ptr<Sistema> controlador);
    inline std::shared_ptr<Sistema> aceleratedNuevo(std::shared_ptr<Sistema> t, QVector<data_box *> *datosCortesBoundaries);
    inline std::shared_ptr<Sistema> aceleratedAntiguo(std::shared_ptr<Sistema> t, QVector<data_box *> *datosCortesBoundaries);



    inline void comprobarVariables (std::shared_ptr<Sistema> controlador);
    inline FC::return_bisection split_box_bisection_avanced(std::shared_ptr<Sistema> current_controlador);
    inline FC::return_bisection split_box_bisection(std::shared_ptr<Sistema> current_controlador);


    std::shared_ptr<Sistema> planta;
    std::shared_ptr<Sistema> controlador;
    QVector <qreal> * omega;
    std::shared_ptr<DatosBound> boundaries;
    std::shared_ptr<DatosBound> boundariesAux;
    std::shared_ptr<Natura_Interval_extension> conversion;
    std::unique_ptr<ListaOrdenada> lista;
    qreal epsilon;

    std::shared_ptr<Sistema> controlador_retorno;
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
    
    FC::return_bisection (Algorithm_primer_articulo::*split_box)(std::shared_ptr<Sistema>);
    data_box * (DeteccionViolacionBoundaries::*deteccionViolacion) (cinterval, std::shared_ptr<DatosBound>, qint32);

    std::shared_ptr<Sistema> (Algorithm_primer_articulo::*analisis)(std::shared_ptr<Sistema> v, QVector<data_box *> *datosCortesBoundaries);

    bool Nyquist;


    std::unique_ptr<DeteccionViolacionBoundaries> deteccion;
    QVector <cxsc::complex> * plantas_nominales;
    QVector <std::complex <qreal>> * plantas_nominales2;


    QVector <QVector<qreal> *> * f;
};

#endif

