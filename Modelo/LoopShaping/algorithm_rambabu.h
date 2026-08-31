#ifndef ALGORITHM_RAMBABU_H
#define ALGORITHM_RAMBABU_H


#include <QVector>
#include <QHash>
#include <QMap>

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
#include "interval.hpp"
#include "EstructuraDatos/arbol_exp.h"


class Algorithm_rambabu
{
public:
    Algorithm_rambabu();
    ~Algorithm_rambabu();


    void set_datos(LtiSystem *planta, LtiSystem *controlador, QVector<qreal> * omega, BoundaryData *boundaries,
                                     qreal epsilon, QVector<QVector<QVector<QPointF> *> *> *reunBoun, bool depuracion,
                                      QVector <QVector <std::complex <qreal> > * > * temp, QVector <tools::dBND *> * espe);

    bool init_algorithm();

    LtiSystem * getControlador();


private:

    tools::flags_box feasibility_test (cinterval box, qreal omega);
    tools::flags_box check_box_feasibility(LtiSystem *v);
    LtiSystem *acelerated(LtiSystem * controlador);

    bool crear_ecuaciones(LtiSystem *controlador);

    //Funciones para crear las ecuaciones
    QVector<QVector<QString> *> *kganacia(LtiSystem * controlador);
    QVector<QVector<QString> *> * knganancia (LtiSystem * controlador);

    LtiSystem * planta;
    LtiSystem * controlador;
    QVector <qreal> * omega;
    BoundaryData * boundaries;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;
    //QVector<QMap<QString, QVector<QVector <interval> *> *> * ecuaciones;
    QVector <QVector <std::complex <qreal> > * > * temp;
    QVector <tools::dBND *> * espe;

    LtiSystem * controlador_retorno;

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
