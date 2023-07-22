#ifndef ALGORITHM_RAMBABU_H
#define ALGORITHM_RAMBABU_H


#include <QVector>
#include <QHash>
#include <QMap>

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
#include "interval.hpp"
#include "EstructuraDatos/arbol_exp.h"


class Algorithm_rambabu
{
public:
    Algorithm_rambabu();
    ~Algorithm_rambabu();


    void set_datos(Sistema *planta, Sistema *controlador, QVector<qreal> * omega, DatosBound *boundaries,
                                     qreal epsilon, QVector<QVector<QVector<QPointF> *> *> *reunBoun, bool depuracion,
                                      QVector <QVector <std::complex <qreal> > * > * temp, QVector <tools::dBND *> * espe);

    bool init_algorithm();

    Sistema * getControlador();


private:

    tools::flags_box feasibility_test (cinterval box, qreal omega);
    tools::flags_box check_box_feasibility(Sistema *v);
    Sistema *acelerated(Sistema * controlador);

    bool crear_ecuaciones(Sistema *controlador);

    //Funciones para crear las ecuaciones
    QVector<QVector<QString> *> *kganacia(Sistema * controlador);
    QVector<QVector<QString> *> * knganancia (Sistema * controlador);

    Sistema * planta;
    Sistema * controlador;
    QVector <qreal> * omega;
    DatosBound * boundaries;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;
    //QVector<QMap<QString, QVector<QVector <interval> *> *> * ecuaciones;
    QVector <QVector <std::complex <qreal> > * > * temp;
    QVector <tools::dBND *> * espe;

    Sistema * controlador_retorno;

    qreal minimo_boundaries;

    DeteccionViolacionBoundaries * deteccion;

    QVector<QVector<QVector<QPointF> *> *> * reunBounHash;

    QVector <bool> * metaDatosArriba;
    QVector <bool> * metaDatosAbierto;

    qint32 tamFas;

    bool depuracion;

    QVector <cxsc::complex> * plantas_nominales;
    QVector <std::complex <qreal>> * plantas_nominales2;


};

#endif // ALGORITHM_RAMBABU_H
