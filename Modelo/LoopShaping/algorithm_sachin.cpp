#include "algorithm_sachin.h"
#include<iostream>
#include<stdlib.h>
#include<time.h>

using namespace tools;
using namespace cxsc;
using namespace FC;

Algorithm_sachin::Algorithm_sachin() {

}

Algorithm_sachin::~Algorithm_sachin() {

}

void Algorithm_sachin::set_datos(LtiSystem * planta, LtiSystem * controlador, QVector<qreal> *omega, DatosBound * boundaries,
                                 qreal epsilon, QVector<QVector<QVector<QPointF> *> *> * reunBounHash) {


    this->planta = planta;
    this->controlador = controlador->clone();
    this->omega = omega;
    this->boundaries = boundaries;
    this->epsilon = epsilon;
    this->reunBounHash = reunBounHash;

    this->metaDatosArriba = boundaries->getMetaDatosArriba();
    this->metaDatosAbierto = boundaries->getMetaDatosAbierta();

    this->tamFas = boundaries->getTamFas() - 1;
    this->depuracion = true;
}


//Función principal del algoritmo

bool Algorithm_sachin::init_algorithm() {

    using namespace std;

    lista = new ListaOrdenada();

    conversion = new Natura_Interval_extension();
    deteccion = new DeteccionViolacionBoundaries();

    plantas_nominales = new QVector <cxsc::complex> ();

    foreach (qreal o, *omega) {
        std::complex <qreal> c = planta->evaluate(o);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }


    check_box_feasibility(controlador);


    while (true) {
        
        
        if (lista->esVacia()) {
            menerror("El espacio de parámetros inicial del controlador no es válido.", "Loop Shaping");

            delete conversion;
            delete lista;
            delete deteccion;

            return false;
        }

        Tripleta * tripleta = static_cast<Tripleta *>(lista->recuperarPrimero());
        lista->borrarPrimero();
        
        if (tripleta->getFlags() == feasible || if_less_epsilon(tripleta->getSistema(), this->epsilon, omega, conversion, plantas_nominales)) {
            if (tripleta->getFlags() == ambiguous) {
                controlador_retorno = guardarControlador(tripleta->getSistema(), false);
            } else {
                controlador_retorno = guardarControlador(tripleta->getSistema(), true);
            }

            delete conversion;
            delete lista;
            delete tripleta;
            delete deteccion;

            return true;
        }

        //Split blox
        struct return_bisection retur = split_box_bisection(tripleta->getSistema());

        tripleta->noBorrar2();
        delete tripleta;

        check_box_feasibility(retur.v1);
        check_box_feasibility(retur.v2);
    }


    return true;
}


//Función que retorna el controlador.

LtiSystem * Algorithm_sachin::getControlador() {
    return controlador_retorno;
}


//Función que comprueba si la caja actual es feasible, infeasible o ambiguous.

inline void Algorithm_sachin::check_box_feasibility(LtiSystem * controlador) {

    using namespace std;

    data_box * datos;

    flags_box flag_final = feasible;

    qint32 contador = 0;
    depuracion = true;
    cinterval caja;
    bool penalizacion = false;

    foreach(qreal o, *omega) {

        caja = conversion->get_box(controlador, o, plantas_nominales->at(contador), false);

        datos = deteccion->deteccionViolacionCajaNi(caja, boundaries, contador);

        if (datos->getFlag() == infeasible) {
            delete controlador;
            delete datos;

            return;
        }

        if (datos->getFlag() == ambiguous) {
            flag_final = ambiguous;

            controlador = acelerated(controlador, datos->getMinimoxMaximos()->at(0), datos->getMinimoxMaximos()->at(1), o, contador, datos->isUniArriba());
        }

        if (o == 2 && SupIm(caja) < -180){
            penalizacion = true;
        }

        delete datos;

        contador++;
    }

    lista->insertar(new Tripleta(penalizacion ? controlador->gain()->range().x() + 100 : controlador->gain()->range().x(), controlador, flag_final));

}


//Función que recorta la caja.

inline LtiSystem * Algorithm_sachin::acelerated(LtiSystem *v, qreal minimo_boundarie, qreal maximo_boundarie, qreal o, qint32 contador, bool arriba) {

    if (!arriba){

        Parameter * min_k_lineal = new Parameter(v->gain()->range().x());
        qreal min_k_db = 20 * log10(min_k_lineal->range().x());

        LtiSystem * G_k_min = v->create(v->name(), v->numerator(), v->denominator(),
                                      min_k_lineal, v->delay());


        qreal mag_min_db = _double(SupRe(conversion->get_box(G_k_min, o, plantas_nominales->at(contador), false)));

        delete min_k_lineal;
        G_k_min->releaseOwnership();
        delete G_k_min;


        if (mag_min_db < minimo_boundarie) {

            qreal Kb_db = min_k_db + (minimo_boundarie - mag_min_db);

            qreal Kb_lineal = pow(10, Kb_db / 20);

            LtiSystem * nuevo_sistema = v->create(v->name(), v->numerator(), v->denominator(),
                                                new Parameter("kv", QPointF(Kb_lineal, v->gain()->range().y()), Kb_lineal, "kv"), v->delay());

            delete v->gain();
            v->releaseOwnership();
            delete v;

            v = nuevo_sistema;
        }
    } /*else {

        Parameter * max_k_lineal = new Parameter(v->gain()->range().y());
        qreal max_k_db = 20 * log10(max_k_lineal->range().y());

        LtiSystem * G_k_max = v->create(v->name(), v->numerator(), v->denominator(),
                                      max_k_lineal, v->delay());


        qreal mag_max_db = conversion->get_box(G_k_max, o, plantas_nominales->at(contador), false).re.sup;

        delete max_k_lineal;
        G_k_max->borrar();
        delete G_k_max;


        if (mag_max_db > maximo_boundarie) {

            qreal Kb_db = max_k_db + (maximo_boundarie - mag_max_db);

            qreal Kb_lineal = pow(10, Kb_db / 20);

            LtiSystem * nuevo_sistema = v->create(v->name(), v->numerator(), v->denominator(),
                                                new Parameter("kv", QPointF(v->gain()->range().x(), Kb_lineal), Kb_lineal, "kv"), v->delay());

            delete v->gain();
            v->borrar();
            delete v;

            v = nuevo_sistema;
        }
    }*/

    return v;
}
