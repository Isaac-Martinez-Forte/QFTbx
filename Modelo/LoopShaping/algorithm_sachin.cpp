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

void Algorithm_sachin::set_datos(std::shared_ptr<Sistema> planta, std::shared_ptr<Sistema> controlador, QVector<qreal> *omega, std::shared_ptr<DatosBound> boundaries,
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

    lista = make_unique<ListaOrdenada>();

    conversion = make_shared<Natura_Interval_extension>();
    deteccion = make_unique<DeteccionViolacionBoundaries>();

    plantas_nominales = new QVector <cxsc::complex> ();

    foreach (qreal o, *omega) {
        std::complex <qreal> c = planta->getPunto(o);
        plantas_nominales->append(cxsc::complex(c.real(), c.imag()));
    }


    check_box_feasibility(controlador);


    while (true) {
        
        
        if (lista->esVacia()) {
            menerror("El espacio de parámetros inicial del controlador no es válido.", "Loop Shaping");

            return false;
        }

        std::shared_ptr<Tripleta> tripleta = std::static_pointer_cast<Tripleta>(lista->recuperarPrimero());
        lista->borrarPrimero();
        
        if (tripleta->getFlags() == feasible || if_less_epsilon(tripleta->getSistema(), this->epsilon, omega, conversion, plantas_nominales)) {
            if (tripleta->getFlags() == ambiguous) {
                controlador_retorno = guardarControlador(tripleta->getSistema(), false);
            } else {
                controlador_retorno = guardarControlador(tripleta->getSistema(), true);
            }

            return true;
        }

        //Split blox
        struct return_bisection retur = split_box_bisection(tripleta->getSistema());

        tripleta->noBorrar2();

        check_box_feasibility(retur.v1);
        check_box_feasibility(retur.v2);
    }


    return true;
}


//Función que retorna el controlador.

std::shared_ptr<Sistema> Algorithm_sachin::getControlador() {
    return controlador_retorno;
}


//Función que comprueba si la caja actual es feasible, infeasible o ambiguous.

inline void Algorithm_sachin::check_box_feasibility(std::shared_ptr<Sistema> controlador) {

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

    lista->insertar(std::make_shared<Tripleta>(penalizacion ? controlador->getK()->getRango().x() + 100 : controlador->getK()->getRango().x(), controlador, flag_final));

}


//Función que recorta la caja.

inline std::shared_ptr<Sistema> Algorithm_sachin::acelerated(std::shared_ptr<Sistema> v, qreal minimo_boundarie, qreal maximo_boundarie, qreal o, qint32 contador, bool arriba) {

    if (!arriba){

        Var * min_k_lineal = new Var(v->getK()->getRango().x());
        qreal min_k_db = 20 * log10(min_k_lineal->getRango().x());

        std::shared_ptr<Sistema> G_k_min = v->invoke(v->getNombre(), v->getNumerador(), v->getDenominador(),
                                      min_k_lineal, v->getRet());


        qreal mag_min_db = _double(SupRe(conversion->get_box(G_k_min, o, plantas_nominales->at(contador), false)));

        delete min_k_lineal;
        G_k_min->noBorrar();


        if (mag_min_db < minimo_boundarie) {

            qreal Kb_db = min_k_db + (minimo_boundarie - mag_min_db);

            qreal Kb_lineal = pow(10, Kb_db / 20);

            std::shared_ptr<Sistema> nuevo_sistema = v->invoke(v->getNombre(), v->getNumerador(), v->getDenominador(),
                                                new Var("kv", QPointF(Kb_lineal, v->getK()->getRango().y()), Kb_lineal, "kv"), v->getRet());

            delete v->getK();
            v->noBorrar();

            v = nuevo_sistema;
        }
    } /*else {

        Var * max_k_lineal = new Var(v->getK()->getRango().y());
        qreal max_k_db = 20 * log10(max_k_lineal->getRango().y());

        std::shared_ptr<Sistema> G_k_max = v->invoke(v->getNombre(), v->getNumerador(), v->getDenominador(),
                                      max_k_lineal, v->getRet());


        qreal mag_max_db = conversion->get_box(G_k_max, o, plantas_nominales->at(contador), false).re.sup;

        delete max_k_lineal;
        G_k_max->borrar();
        delete G_k_max;


        if (mag_max_db > maximo_boundarie) {

            qreal Kb_db = max_k_db + (maximo_boundarie - mag_max_db);

            qreal Kb_lineal = pow(10, Kb_db / 20);

            std::shared_ptr<Sistema> nuevo_sistema = v->invoke(v->getNombre(), v->getNumerador(), v->getDenominador(),
                                                new Var("kv", QPointF(v->getK()->getRango().x(), Kb_lineal), Kb_lineal, "kv"), v->getRet());

            delete v->getK();
            v->borrar();
            delete v;

            v = nuevo_sistema;
        }
    }*/

    return v;
}
