#ifndef ALGORITHM_NANDKISHOR_H
#define ALGORITHM_NANDKISHOR_H


#include <QVector>
#include <QHash>

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

#include "Modelo/LoopShaping/funcionescomunes.h"

#include "mpParser.h"


class Algorithm_nandkishor
{
public:
    Algorithm_nandkishor();
    ~Algorithm_nandkishor();


    void set_datos (LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, BoundaryData * boundaries,
                     qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBoun, qreal delta, qint32 inicializacion );

    bool init_algorithm();

    LtiSystem * getControlador();


private:

    enum tipoInicializacion {centro, superior, aleatorio};

    inline flags_box check_box_feasibility ( QVector <qreal> * nume, QVector <qreal> * deno, qreal k,
            qreal ret );
    inline LtiSystem * acelerated(LtiSystem * v, QVector<data_box *> *datosCortesBoundaries);
    inline void local_optimization ( LtiSystem * controlador );
    inline LtiSystem * get_minimo_sistema ( LtiSystem *v );
    inline qreal busqueda_local ( qreal delta, LtiSystem *controlador );

    inline tools::flags_box check_box_feasibility ( LtiSystem *controlador );

    /*inline qreal get_k (LtiSystem *controlador, QVector<qreal> *nume_sup, QVector<qreal> *deno_inf, qreal minimo_boundarie,
                         std::complex <qreal> p0, qreal omega, qreal k_min, qreal k_max);
    inline QVector<qreal> * get_nume_kganancia (LtiSystem * controlador, QVector<qreal> *nume_sup, QVector<qreal> *deno_inf, qreal k_max, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega, QVector<qreal> * nume_inf );
    inline QVector<qreal> * get_deno_kganancia (LtiSystem * controlador, QVector<qreal> *nume_sup, QVector<qreal> *deno_inf, qreal k_max, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega , QVector<qreal> *deno_sup);

    inline QVector<qreal> * get_nume_knganancia ( LtiSystem * controlador, QVector<qreal> *nume, QVector<qreal> *deno, qreal k, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega, QVector<qreal> * nume_bajo );
    inline QVector<qreal> * get_deno_knganancia (LtiSystem * controlador, QVector<qreal> *nume, QVector<qreal> *deno_inf, QVector<qreal> *deno_sup, qreal k, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega );

    inline QVector<qreal> * get_nume_cpol ( LtiSystem * controlador, QVector<qreal> *nume, QVector<qreal> *deno, qreal k, qreal minimo_boundarie,
                                            std::complex <qreal> p0, qreal omega, QVector<qreal> * nume_bajo );
    inline QVector<qreal> * get_deno_cpol ( LtiSystem * controlador, QVector<qreal> *nume, QVector<qreal> *deno, qreal k, qreal minimo_boundarie,
                                            std::complex <qreal> p0, qreal omega );*/

    inline qint32 crearVectores ( LtiSystem * controlador, QVector <qreal> * numerador, QVector <qreal> * denominador, QVector<qreal> *k,
                                  QVector<QVector<qreal> * > * variables, qreal delta, QVector <qreal> * numeNominales,
                                  QVector <qreal> * denoNominales, qreal kNominal );


    /*inline qreal get_k_max(LtiSystem *controlador, QVector <qreal> * nume_inf, QVector <qreal> * deno_sup,
                                             qreal maximo_boundarie,
                                             std::complex<qreal> p0, qreal omega, qreal k_min, qreal k_max);


    inline QVector<qreal> * get_nume_kganancia_max(LtiSystem *controlador, QVector <qreal> * nume_inf, QVector <qreal> * deno_sup,
                                                                     qreal k_min,
                                                                     qreal maximo_boundarie, std::complex<qreal> p0, qreal omega,
                                                                     QVector <qreal> * nume_sup);

    inline QVector<qreal> * get_deno_kganancia_max(LtiSystem *controlador, QVector <qreal> * nume_inf, QVector <qreal> * deno_sup,
                                                                     qreal k_min, qreal maximo_boundarie, std::complex<qreal> p0, qreal omega, QVector <qreal> * deno_inf);*/

    inline qreal log10 (qreal a);



    inline qreal inicializacion ( LtiSystem * controlador, QVector <qreal> * numerador, QVector <qreal> * denominador, tipoInicializacion tipo );

    inline void comprobarVariables ( LtiSystem * controlador );

    LtiSystem * planta;
    LtiSystem * controlador;
    LtiSystem * controlador_inicial;
    QVector <qreal> * omega;
    BoundaryData * boundaries;
    NaturalIntervalExtension * conversion;
    ListaOrdenada * lista;

    LtiSystem * controlador_retorno;
    qreal current_omega;
    qreal epsilon;
    qreal delta;

    qreal mejor_k;
    QVector <qreal> * anterior_sis_min;

    qreal minimo_boundaries;

    QVector<QVector<QVector<QPointF> *> *> * reunBounHash;

    QVector <bool> * metaDatosArriba;
    QVector <bool> * metaDatosAbierto;

    bool isVariableNume;
    bool isVariableDeno;

    qint32 tamFas;

    bool depuracion;

    QVector <complex> * plantas_nominales;
    QVector <std::complex <qreal> > * plantas_nominales2;

    tipoInicializacion ini;

    DeteccionViolacionBoundaries * deteccion;
};

#endif // ALGORITHM_NANDKISHOR_H
