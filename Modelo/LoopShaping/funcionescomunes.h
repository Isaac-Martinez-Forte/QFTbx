#ifndef FUNCIONESCOMUNES_H
#define FUNCIONESCOMUNES_H


#include <QVector>
#include <QPointF>

#include "src/core/system/lti_system.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/boundaries/boundary_data.h"
#include "Modelo/LoopShaping/NaturalIntervalExtension/natural_interval_extension.h"
#include "Modelo/LoopShaping/DeteccionViolacionBoundaries/deteccionviolacionboundaries.h"
#include "Modelo/LoopShaping/EstructuraDatos/listaordenada.h"
#include "Modelo/LoopShaping/EstructuraDatos/tripleta2.h"

#include "cinterval.hpp"
#include <complex>

using namespace tools;
using namespace cxsc;

namespace FC {

struct return_bisection {
    LtiSystem * v1;
    LtiSystem * v2;
    bool descartado;
};

struct return_bisection2 {
    Tripleta2 * t1;
    Tripleta2 * t2;
    bool descartado;
};

enum diagrama {Nichol = false, Nyquist = true};

//Extracts a point controller from a box. With x = true, the lower corner
//of every parameter (a feasible box realises its optimum gain there).
//With x = false, the corner that the monotonicity of the Nichols
//projection makes feasible for an epsilon-small ambiguous box sitting on
//a boundary whose allowed side is up (the anti-blocking rule, QFTbx
//thesis sec. 3.1): maximum gain and zeros push the box up, but poles push
//it DOWN, so poles take their minimum (the historical code took every
//maximum, stepping AWAY from the allowed side in the pole directions).
inline LtiSystem * guardarControlador(LtiSystem *controlador, bool x) {


    QVector <Parameter *> * nume = controlador->numerator();
    QVector <Parameter *> * numerador = new QVector <Parameter *> ();

    foreach (Parameter * v, *nume) {
        if (v->isUncertain()){
            if (x){
                numerador->append(new Parameter (v->range().x()));
            }else {
                numerador->append(new Parameter (v->range().y()));
            }
        } else {
            numerador->append(new Parameter (v->nominal()));
        }
    }

    QVector <Parameter *> * deno = controlador->denominator();
    QVector <Parameter *> * denominador = new QVector <Parameter *> ();

    foreach (Parameter * v, *deno) {
        if (v->isUncertain()){
            //Poles always take the lower corner: a larger pole moves the
            //projection towards the forbidden side.
            denominador->append(new Parameter (v->range().x()));
        } else {
            denominador->append(new Parameter (v->nominal()));
        }
    }

    Parameter * k;

    if (x){
        k = new Parameter (controlador->gain()->range().x());
    } else {
        k = new Parameter (controlador->gain()->range().y());
    }



    LtiSystem * s = controlador->create(controlador->name(), numerador, denominador,
                                      k, new Parameter ((qreal) 0));

    return s;
}

inline bool if_less_epsilon(LtiSystem * controlador, qreal epsilon, QVector <qreal> * omega,
                            NaturalIntervalExtension *conversion, QVector <complex> * plantas_nominales) {

    cinterval box;
    for (qint32 i = 0; i < omega->size(); i++){
        box = conversion->nicholsBox(controlador, omega->at(i), plantas_nominales->at(i), false);

        if ((cxsc::diam(Re(box)) >= epsilon) || (cxsc::diam(Im(box)) >= epsilon)) {
            return false;
        }
    }

    return true;
}

//Función que divide la caja en dos.

inline return_bisection split_box_bisection(LtiSystem *current_controlador) {

    QVector <Parameter *> * numerador = current_controlador->numerator();
    QVector <Parameter *> * denominador = current_controlador->denominator();

    QVector <Parameter *> * numeradorCopia = new QVector <Parameter *> ();
    QVector <Parameter *> * denominadorCopia = new QVector <Parameter *> ();

    Parameter * k = current_controlador->gain();
    Parameter * ret = current_controlador->delay();

    QString nombre = current_controlador->name();

    //Variables contador;
    qint32 mayor_pos = -1;
    qreal mayor_rango = -1;

    //Variables auxiliares
    qreal lon = 0;
    qreal cont = 0;

    //Sistemas hijos creados

    LtiSystem * v1, * v2;
    struct FC::return_bisection retur;


    //Bucle del numerador
    Parameter * v;
    for (qint32 i = 0; i < numerador->size(); i++) {
        v = numerador->at(i);
        numeradorCopia->append(v->clone());
        if (v->isUncertain()) {

            lon = v->range().y() - v->range().x();

            if (lon > mayor_rango) {
                mayor_pos = cont;
                mayor_rango = lon;
            }
        }
        cont++;
    }

    //Bucle del denominador
    for (qint32 i = 0; i < denominador->size(); i++) {
        v = denominador->at(i);
        denominadorCopia->append(v->clone());
        if (v->isUncertain()) {

            lon = v->range().y() - v->range().x();

            if (lon > mayor_rango) {
                mayor_pos = cont;
                mayor_rango = lon;
            }
        }
        cont++;
    }

    //Estudiamos la k
    if (k->isUncertain()) {

        lon = k->range().y() - k->range().x();

        if (lon > mayor_rango) {
            mayor_pos = -1;
            mayor_rango = lon;
        }
    }


    if (mayor_pos == -1) {
        //The halves keep the parameter's NAME: the ICSP constraint trees
        //address the variables by name (the historical code renamed the
        //gain to "kv" and erased the zero/pole names).
        qreal dis = k->range().x();
        Parameter * k1 = new Parameter(k->name(), QPointF(dis, dis + (mayor_rango / 2)), dis);
        dis += mayor_rango / 2;
        Parameter * k2 = new Parameter(k->name(), QPointF(dis, k->range().y()), dis);

        delete k;

        v1 = current_controlador->create(nombre, numerador, denominador, k1, ret);
        v2 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, k2, ret->clone());
    } else if (mayor_pos < numerador->size()) {

        Parameter * variable = numerador->at(mayor_pos);

        qreal dis = variable->range().x();

        numeradorCopia->replace(mayor_pos, new Parameter(variable->name(), QPointF(dis, dis + mayor_rango / 2), dis));

        dis += mayor_rango / 2;
        numerador->replace(mayor_pos, new Parameter(variable->name(), QPointF(dis, variable->range().y()), dis));


        v1 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, k->clone(), ret->clone());
        v2 = current_controlador->create(nombre, numerador, denominador, k, ret);

        delete variable;

    } else {
        mayor_pos -= numerador->size();

        Parameter * variable = denominador->at(mayor_pos);
        qreal dis = variable->range().x();

        denominadorCopia->replace(mayor_pos, new Parameter(variable->name(), QPointF(dis, dis + mayor_rango / 2), dis));

        dis += mayor_rango / 2;
        denominador->replace(mayor_pos, new Parameter(variable->name(), QPointF(dis, variable->range().y()), dis));

        v1 = current_controlador->create(nombre, numeradorCopia, denominadorCopia, k->clone(), ret->clone());
        v2 = current_controlador->create(nombre, numerador, denominador, k, ret);

        delete variable;

    }


    retur.v1 = v1;
    retur.v2 = v2;
    retur.descartado = false;
    return retur;
}

} // fin namespace

#endif
