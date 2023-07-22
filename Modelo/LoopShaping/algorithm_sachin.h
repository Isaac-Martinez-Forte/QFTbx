#ifndef ALGORITHM_SACHIN_H
#define ALGORITHM_SACHIN_H

#include <QVector>
#include <QHash>
#include <cmath>

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


class Algorithm_sachin
{
public:
    Algorithm_sachin();
    ~Algorithm_sachin();

    void set_datos(Sistema * planta, Sistema * controlador, QVector<qreal> *omega, DatosBound * boundaries,
                    qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash);

    bool init_algorithm();

    Sistema * getControlador();


private:

    inline void check_box_feasibility(Sistema *controlador);
    inline Sistema *acelerated(Sistema * v, qreal minimo_boundarie, qreal maximo_boundarie, qreal o, qint32 contador, bool arriba);

    Sistema * planta;
    Sistema * controlador;
    QVector <qreal> * omega;
    DatosBound * boundaries;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;

    Sistema * controlador_retorno;
    qreal minimo_boundaries;

    QVector<QVector<QVector<QPointF> *> *> * reunBounHash;

    QPointF interseccion (QPointF uno, QPointF dos);

    QVector <bool> * metaDatosArriba;
    QVector <bool> * metaDatosAbierto;

    qint32 tamFas;

    bool depuracion;

    DeteccionViolacionBoundaries * deteccion;
    QVector <complex> * plantas_nominales;

};

#endif // ALGORITHM_SACHIN_H
