#ifndef ALGORITHM_SEGUNDO_ARTICULO_H
#define ALGORITHM_SEGUNDO_ARTICULO_H


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
#include "EstructuraDatos/datosfeasible.h"
#include "EstructuraDatos/tripleta.h"
#include "EstructuraDatos/tripleta2.h"
#include "EstructuraDatos/etapas.h"
#include "Modelo/Herramientas/tools.h"
#include "Modelo/EstructuraSistema/cpolinomios.h"
#include "Modelo/EstructuraSistema/kganancia.h"
#include "Modelo/EstructuraSistema/knganancia.h"
#include "DeteccionViolacionBoundaries/deteccionviolacionboundaries.h"
#include "EstructuraDatos/listaordenada.h"
#include "funcionescomunes.h"
#include "GUI/viewboundreun.h"
#include "interval.hpp"
#include "EstructuraDatos/n.h"

#include "QLogger.h"

class Algorithm_segundo_articulo: public QObject
{
    Q_OBJECT

public:
    Algorithm_segundo_articulo();
    ~Algorithm_segundo_articulo();

    void set_datos(Sistema * planta, Sistema * controlador, QVector<qreal> *omega, DatosBound * boundaries,
                    qreal epsilon);

    bool init_algorithm();

    Sistema * getControlador();

private:

    inline void comprobarVariables ( Sistema * controlador);
    inline bool analizar(Tripleta2 *tripleta);
    inline bool aplicarMejoras(Tripleta2 *tripleta);
    inline Sistema * busquedaMejorGanancia (Tripleta2 * tripleta);
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

    Sistema * planta;
    Sistema * controlador;
    Sistema * mejorSolucion;
    Sistema * controlador_retorno;

    QVector <qreal> * omega;
    DatosBound * boundaries;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;
    qreal epsilon;

    DeteccionViolacionBoundaries * deteccion;
    QVector <cxsc::complex> * plantas_nominales;
    QVector <std::complex <qreal>> * plantas_nominales2;

    data_box * (DeteccionViolacionBoundaries::*deteccionViolacion) (cinterval, DatosBound *, qint32, Etapas);

    bool isVariableNume;
    bool isVariableDeno;

    qint32 frecuenciaPrincipal;

    bool cambioEtapaFinal;
};

#endif // ALGORITHM_SEGUNDO_ARTICULO_H
