#ifndef ALGORITHM_MC_THESIS_H
#define ALGORITHM_MC_THESIS_H


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
#include "EstructuraDatos/datosfeasible.h"
#include "EstructuraDatos/tripleta.h"
#include "EstructuraDatos/tripleta2.h"
#include "EstructuraDatos/etapas.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/system/polynomial_form.h"
#include "src/core/system/zero_pole_gain.h"
#include "src/core/system/time_constant_gain.h"
#include "DeteccionViolacionBoundaries/deteccionviolacionboundaries.h"
#include "EstructuraDatos/listaordenada.h"
#include "funcionescomunes.h"
#include "interval.hpp"
#include "EstructuraDatos/n.h"

class AlgorithmMcThesis: public QObject
{
    Q_OBJECT

public:
    AlgorithmMcThesis();
    ~AlgorithmMcThesis();

    void set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, BoundaryData * boundaries,
                    qreal epsilon);

    bool init_algorithm();

    LtiSystem * getControlador();

private:

    inline void comprobarVariables ( LtiSystem * controlador);
    inline bool analizar(Tripleta2 *tripleta);
    inline bool aplicarMejoras(Tripleta2 *tripleta);
    inline LtiSystem * busquedaMejorGanancia (Tripleta2 * tripleta);
    inline Tripleta2 * recortesInfeasible(Tripleta2 * tripleta);
    inline Tripleta2 * recortesFeasible(Tripleta2 * tripleta);
    inline Tripleta2 * analisisFeasible(Tripleta2 * tripleta);

    inline Tripleta2 * beneficioEstimado (Tripleta2 * tripleta);

    inline FC::return_bisection2 biseccion (Tripleta2 * tripleta);
    inline FC::return_bisection2 biseccionArea(Tripleta2 *tripleta);
    inline FC::return_bisection2 biseccionMag(Tripleta2 *tripleta);
    inline FC::return_bisection2 biseccionFas(Tripleta2 *tripleta);
    inline FC::return_bisection2 biseccionArbol(Tripleta2 *tripleta);

    inline Tripleta2 * calculoTerminosControlador (Tripleta2* controlador);

    LtiSystem * planta;
    LtiSystem * controlador;
    LtiSystem * mejorSolucion;
    LtiSystem * controlador_retorno;

    QVector <qreal> * omega;
    BoundaryData * boundaries;
    NaturalIntervalExtension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;

    DeteccionViolacionBoundaries * deteccion;
    QVector <cxsc::complex> * plantas_nominales;
    QVector <std::complex <qreal>> * plantas_nominales2;

    data_box * (DeteccionViolacionBoundaries::*deteccionViolacion) (cinterval, BoundaryData *, qint32, Etapas);

    bool isVariableNume;
    bool isVariableDeno;

    qint32 frecuenciaPrincipal;

    bool cambioEtapaFinal;
};

#endif // ALGORITHM_MC_THESIS_H
