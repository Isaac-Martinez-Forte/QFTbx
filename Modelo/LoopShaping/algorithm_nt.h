#ifndef ALGORITHM_NT_H
#define ALGORITHM_NT_H

#include <QVector>
#include <QHash>
#include <cmath>

#include "src/core/boundaries/boundary_data.h"
#include "src/core/system/lti_system.h"
#include "NaturalIntervalExtension/natural_interval_extension.h"
#include "EstructuraDatos/tripleta.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "DeteccionViolacionBoundaries/deteccionviolacionboundaries.h"
#include "nominal_stability_checker.h"
#include "EstructuraDatos/listaordenada.h"

#include "funcionescomunes.h"



class AlgorithmNt
{
public:
    AlgorithmNt();
    ~AlgorithmNt();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, BoundaryData * boundaries,
                    qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash);

    bool init_algorithm();

    LtiSystem * getControlador();


private:

    inline void check_box_feasibility(LtiSystem *controlador);
    inline LtiSystem *acelerated(LtiSystem * v, qreal minimo_boundarie, qreal o, qint32 contador, bool arriba);
    inline bool feasibleGainFrom(LtiSystem * v, qreal maximo_boundarie, cxsc::cinterval caja,
                                 qreal o, qint32 contador, qreal & from);

    LtiSystem * planta;
    LtiSystem * controlador;
    QVector <qreal> * omega;
    BoundaryData * boundaries;
    NaturalIntervalExtension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;

    LtiSystem * controlador_retorno;
    qreal minimo_boundaries;

    QVector<QVector<QVector<QPointF> *> *> * reunBounHash;

    QPointF interseccion (QPointF uno, QPointF dos);

    QVector <bool> * metaDatosArriba;
    QVector <bool> * metaDatosAbierto;

    qint32 tamFas;

    DeteccionViolacionBoundaries * deteccion;
    NominalStabilityChecker * stability;
    QVector <complex> * plantas_nominales;

};

#endif // ALGORITHM_NT_H
