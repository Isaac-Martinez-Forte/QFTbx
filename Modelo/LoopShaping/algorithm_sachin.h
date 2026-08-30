#ifndef ALGORITHM_SACHIN_H
#define ALGORITHM_SACHIN_H

#include <QVector>
#include <QHash>
#include <cmath>

#include "Modelo/EstructurasDatos/datosbound.h"
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

#include "GUI/viewboundreun.h"


class Algorithm_sachin
{
public:
    Algorithm_sachin();
    ~Algorithm_sachin();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, DatosBound * boundaries,
                    qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash);

    bool init_algorithm();

    LtiSystem * getControlador();


private:

    inline void check_box_feasibility(LtiSystem *controlador);
    inline LtiSystem *acelerated(LtiSystem * v, qreal minimo_boundarie, qreal maximo_boundarie, qreal o, qint32 contador, bool arriba);

    LtiSystem * planta;
    LtiSystem * controlador;
    QVector <qreal> * omega;
    DatosBound * boundaries;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;

    LtiSystem * controlador_retorno;
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
