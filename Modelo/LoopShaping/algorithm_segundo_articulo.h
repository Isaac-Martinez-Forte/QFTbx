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

class Algorithm_segundo_articulo: public QObject
{
    Q_OBJECT

public:
    Algorithm_segundo_articulo();
    ~Algorithm_segundo_articulo();

    void set_datos(std::shared_ptr<Sistema> planta, std::shared_ptr<Sistema> controlador, QVector<qreal> *omega, std::shared_ptr<DatosBound> boundaries,
                    qreal epsilon);

    bool init_algorithm();

    std::shared_ptr<Sistema> getControlador();

private:

    inline void comprobarVariables (std::shared_ptr<Sistema> controlador);
    inline bool analizar(std::shared_ptr<Tripleta2>tripleta);
    inline bool aplicarMejoras(std::shared_ptr<Tripleta2>tripleta);
    inline std::shared_ptr<Sistema> busquedaMejorGanancia (std::shared_ptr<Tripleta2>tripleta);
    inline std::shared_ptr<Tripleta2>recortesInfeasible(std::shared_ptr<Tripleta2>tripleta);
    inline std::shared_ptr<Tripleta2>recortesFeasible(std::shared_ptr<Tripleta2>tripleta);
    inline std::shared_ptr<Tripleta2>analisisFeasible(std::shared_ptr<Tripleta2>tripleta);

    inline std::shared_ptr<Tripleta2>beneficioEstimado (std::shared_ptr<Tripleta2>tripleta);

    inline FC::return_bisection2 biseccion (std::shared_ptr<Tripleta2>tripleta);
    inline FC::return_bisection2 biseccionArea(std::shared_ptr<Tripleta2>tripleta);
    inline FC::return_bisection2 biseccionMag(std::shared_ptr<Tripleta2>tripleta);
    inline FC::return_bisection2 biseccionFas(std::shared_ptr<Tripleta2>tripleta);
    inline FC::return_bisection2 biseccionArbol(std::shared_ptr<Tripleta2>tripleta);

    inline std::shared_ptr<Tripleta2>calculoTerminosControlador (std::shared_ptr<Tripleta2>controlador);

    std::shared_ptr<Sistema> planta;
    std::shared_ptr<Sistema> controlador;
    std::shared_ptr<Sistema> mejorSolucion;
    std::shared_ptr<Sistema> controlador_retorno;

    QVector <qreal> * omega;
    std::shared_ptr<DatosBound> boundaries;
    std::shared_ptr<Natura_Interval_extension> conversion;
    std::unique_ptr<ListaOrdenada> lista;
    qreal epsilon;

    std::unique_ptr<DeteccionViolacionBoundaries> deteccion;
    QVector <cxsc::complex> * plantas_nominales;
    QVector <std::complex <qreal>> * plantas_nominales2;

    data_box * (DeteccionViolacionBoundaries::*deteccionViolacion) (cinterval, std::shared_ptr<DatosBound>, qint32, Etapas);

    bool isVariableNume;
    bool isVariableDeno;

    qint32 frecuenciaPrincipal;

    bool cambioEtapaFinal;
};

#endif // ALGORITHM_SEGUNDO_ARTICULO_H
