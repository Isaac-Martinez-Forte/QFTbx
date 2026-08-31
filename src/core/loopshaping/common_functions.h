#ifndef QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H
#define QFTBX_LOOPSHAPING_COMMON_FUNCTIONS_H


#include <QVector>
#include <QPointF>

#include "src/core/system/lti_system.h"
#include "Modelo/Herramientas/tools.h"
#include "src/core/boundaries/boundary_data.h"
#include "src/core/loopshaping/natural_interval_extension.h"
#include "src/core/loopshaping/boundary_violation_detector.h"
#include "src/core/loopshaping/ordered_list.h"
#include "src/core/loopshaping/mc_search_node.h"

#include "cinterval.hpp"
#include <complex>

using namespace tools;
using namespace cxsc;

namespace FC {

struct BisectionResult {
    LtiSystem * v1;
    LtiSystem * v2;
    bool descartado;
};

struct McBisectionResult {
    McSearchNode * t1;
    McSearchNode * t2;
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
inline LtiSystem * pointFromBox(LtiSystem *controlador, bool x) {


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

inline bool isEpsilonSmall(LtiSystem * controlador, qreal epsilon, QVector <qreal> * omega,
                            NaturalIntervalExtension *conversion, QVector <complex> * plantas_nominales) {

    cinterval box;
    for (qint32 i = 0; i < omega->size(); i++){
        box = conversion->nicholsBox(controlador, omega->at(i), plantas_nominales->at(i));

        if ((cxsc::diam(Re(box)) >= epsilon) || (cxsc::diam(Im(box)) >= epsilon)) {
            return false;
        }
    }

    return true;
}

//Función que divide la caja en dos.

inline BisectionResult bisectWidestParameter(LtiSystem * box) {

    //Widest uncertain parameter: -1 is the gain, then the numerator and
    //denominator positions.
    qint32 widest = -2;
    qreal width = -1;
    QPointF range;

    if (box->gain()->isUncertain()) {
        range = box->gain()->range();
        widest = -1;
        width = range.y() - range.x();
    }

    qint32 position = 0;

    const auto consider = [&](Parameter * var) {
        if (var->isUncertain() && var->range().y() - var->range().x() > width) {
            widest = position;
            width = var->range().y() - var->range().x();
            range = var->range();
        }
        position++;
    };

    foreach (Parameter * var, *box->numerator()) {
        consider(var);
    }
    foreach (Parameter * var, *box->denominator()) {
        consider(var);
    }

    const qreal middle = range.x() + width / 2;

    //Both children are DEEP copies and the parent stays untouched: its
    //node keeps sole ownership of it (the historical version handed the
    //parent's vectors to the second child, forcing every caller to leak
    //the parent shell to stay safe). The halves keep the parameter's
    //NAME: the ICSP constraint trees address the variables by name.
    const auto half = [&](bool lower) -> LtiSystem * {
        const QPointF halfRange = lower ? QPointF(range.x(), middle)
                                        : QPointF(middle, range.y());

        Parameter * gain = widest == -1
                ? new Parameter(box->gain()->name(), halfRange, halfRange.x(), box->gain()->name())
                : box->gain()->clone();

        qint32 index = 0;

        auto * numerator = new QVector<Parameter*>();
        foreach (Parameter * var, *box->numerator()) {
            numerator->append(index++ == widest
                    ? new Parameter(var->name(), halfRange, halfRange.x())
                    : var->clone());
        }

        auto * denominator = new QVector<Parameter*>();
        foreach (Parameter * var, *box->denominator()) {
            denominator->append(index++ == widest
                    ? new Parameter(var->name(), halfRange, halfRange.x())
                    : var->clone());
        }

        return box->create(box->name(), numerator, denominator, gain,
                           box->delay()->clone());
    };

    BisectionResult retur;
    retur.v1 = half(true);
    retur.v2 = half(false);
    retur.descartado = false;

    return retur;
}

} // fin namespace

#endif
