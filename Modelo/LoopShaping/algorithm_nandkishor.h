#ifndef ALGORITHM_NANDKISHOR_H
#define ALGORITHM_NANDKISHOR_H


#include <QVector>
#include <QHash>

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

#include "Modelo/LoopShaping/funcionescomunes.h"

#include "muparserx/mpParser.h"


class Algorithm_nandkishor
{
public:
    Algorithm_nandkishor();
    ~Algorithm_nandkishor();


    void set_datos (Sistema * planta, Sistema * controlador, QVector<qreal> *omega, std::shared_ptr<DatosBound> boundaries,
                     qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBoun, qreal delta, qint32 inicializacion );

    bool init_algorithm();

    Sistema * getControlador();


private:

    enum tipoInicializacion {centro, superior, aleatorio};

    inline flags_box check_box_feasibility ( QVector <qreal> * nume, QVector <qreal> * deno, qreal k,
            qreal ret );
    inline Sistema * acelerated(Sistema * v, QVector<data_box *> *datosCortesBoundaries);
    inline void local_optimization ( Sistema * controlador );
    inline Sistema * get_minimo_sistema ( Sistema *v );
    inline qreal busqueda_local ( qreal delta, Sistema *controlador );

    inline tools::flags_box check_box_feasibility ( Sistema *controlador );

    /*inline qreal get_k (Sistema *controlador, QVector<qreal> *nume_sup, QVector<qreal> *deno_inf, qreal minimo_boundarie,
                         std::complex <qreal> p0, qreal omega, qreal k_min, qreal k_max);
    inline QVector<qreal> * get_nume_kganancia (Sistema * controlador, QVector<qreal> *nume_sup, QVector<qreal> *deno_inf, qreal k_max, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega, QVector<qreal> * nume_inf );
    inline QVector<qreal> * get_deno_kganancia (Sistema * controlador, QVector<qreal> *nume_sup, QVector<qreal> *deno_inf, qreal k_max, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega , QVector<qreal> *deno_sup);

    inline QVector<qreal> * get_nume_knganancia ( Sistema * controlador, QVector<qreal> *nume, QVector<qreal> *deno, qreal k, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega, QVector<qreal> * nume_bajo );
    inline QVector<qreal> * get_deno_knganancia (Sistema * controlador, QVector<qreal> *nume, QVector<qreal> *deno_inf, QVector<qreal> *deno_sup, qreal k, qreal minimo_boundarie,
            std::complex <qreal> p0, qreal omega );

    inline QVector<qreal> * get_nume_cpol ( Sistema * controlador, QVector<qreal> *nume, QVector<qreal> *deno, qreal k, qreal minimo_boundarie,
                                            std::complex <qreal> p0, qreal omega, QVector<qreal> * nume_bajo );
    inline QVector<qreal> * get_deno_cpol ( Sistema * controlador, QVector<qreal> *nume, QVector<qreal> *deno, qreal k, qreal minimo_boundarie,
                                            std::complex <qreal> p0, qreal omega );*/

    inline qint32 crearVectores ( Sistema * controlador, QVector <qreal> * numerador, QVector <qreal> * denominador, QVector<qreal> *k,
                                  QVector<QVector<qreal> * > * variables, qreal delta, QVector <qreal> * numeNominales,
                                  QVector <qreal> * denoNominales, qreal kNominal );


    /*inline qreal get_k_max(Sistema *controlador, QVector <qreal> * nume_inf, QVector <qreal> * deno_sup,
                                             qreal maximo_boundarie,
                                             std::complex<qreal> p0, qreal omega, qreal k_min, qreal k_max);


    inline QVector<qreal> * get_nume_kganancia_max(Sistema *controlador, QVector <qreal> * nume_inf, QVector <qreal> * deno_sup,
                                                                     qreal k_min,
                                                                     qreal maximo_boundarie, std::complex<qreal> p0, qreal omega,
                                                                     QVector <qreal> * nume_sup);

    inline QVector<qreal> * get_deno_kganancia_max(Sistema *controlador, QVector <qreal> * nume_inf, QVector <qreal> * deno_sup,
                                                                     qreal k_min, qreal maximo_boundarie, std::complex<qreal> p0, qreal omega, QVector <qreal> * deno_inf);*/

    inline qreal log10 (qreal a);



    inline qreal inicializacion ( Sistema * controlador, QVector <qreal> * numerador, QVector <qreal> * denominador, tipoInicializacion tipo );

    inline void comprobarVariables ( Sistema * controlador );

    Sistema * planta;
    Sistema * controlador;
    Sistema * controlador_inicial;
    QVector <qreal> * omega;
    std::shared_ptr<DatosBound> boundaries;
    Natura_Interval_extension * conversion;
    ListaOrdenada * lista;

    Sistema * controlador_retorno;
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
