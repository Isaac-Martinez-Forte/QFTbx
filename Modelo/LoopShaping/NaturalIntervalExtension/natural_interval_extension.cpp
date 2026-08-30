#include "natural_interval_extension.h"

using namespace cxsc;

#define ARG

#define PI 3.1415926535897936
#define dPI 6.283185307179587

Natura_Interval_extension::Natura_Interval_extension()
{

}

Natura_Interval_extension::~Natura_Interval_extension()
{

}

cinterval Natura_Interval_extension::get_box_termino_nume(Parameter * var, qreal w, complex p0) {

    cinterval a = (complex (0, w) + interval(var->range().x(), var->range().y())) * p0;

    interval g = abs(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

    interval theta = _arg(a);

    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box_termino_deno(Parameter * var, qreal w, complex p0) {

    cinterval a = 1 / (complex (0, w) + interval(var->range().x(), var->range().y())) * p0;

    interval g = abs(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

    interval theta = _arg(a);

    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box_termino_k(Parameter * var, complex p0) {

    cinterval a = (interval(var->range().x(), var->range().y())) * p0;

    interval g = abs(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

    interval theta = _arg(a);

    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box(LtiSystem *sistema, qreal w, complex p0, bool nyquist){

    cinterval a;

    /*if (sistema->type() == LtiSystem::SystemType::PolynomialForm){ //TODO arreglar falta formato libre

        a = get_box_cpolinomios(sistema, w);

    } else if (sistema->type() == LtiSystem::SystemType::ZeroPoleGain){
        a = get_box_kganancia(sistema, w);

    } else {
        a =  get_box_knoganacia(sistema, w);
    }*/

    //////////////////////////////////////////////////////////////////

    QVector <Parameter*> * nume = sistema->numerator();
    QVector <Parameter * > * deno = sistema->denominator();

    Parameter * kv = sistema->gain();


    //Creamos el numerador.
    cinterval numerador;
    complex complejo (0, w);


    if (!nume->isEmpty()){
        QVector <Parameter*>::iterator it = nume->begin();

        if ((*it)->isUncertain()){
            numerador = (complejo + interval((*it)->range().x(), (*it)->range().y()));
        }else{
            numerador = (complejo + interval((*it)->nominal()));
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isUncertain()){
                numerador = numerador * (complejo + interval((*it)->range().x(), (*it)->range().y()));
            }else{
                numerador = numerador * (complejo + interval((*it)->nominal()));
            }
        }

    }

    //Creamos el denominador.
    cinterval denominador;

    if (!deno->isEmpty()){
        QVector <Parameter*>::iterator it = deno->begin();

        if ((*it)->isUncertain()){
            denominador = (complejo + interval((*it)->range().x(), (*it)->range().y()));
        }else{
            denominador = (complejo + interval((*it)->nominal()));
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isUncertain()){
                denominador = denominador * (complejo + interval((*it)->range().x(), (*it)->range().y()));
            }else{
                denominador = denominador * (complejo + interval((*it)->nominal()));
            }
        }
    }


    if(kv->isUncertain()){
        a = interval(kv->range().x(), kv->range().y()) * numerador * p0;
    } else{
        a = kv->nominal() * numerador * p0;
    }

    //////////////////////////////////////////////////////////////////

    interval g1 = abs(a);

    if (Inf(g1) == 0){
        SetInf(g1, 0.01);
    }

    interval theta1 = _arg(a);

    interval g2 = abs(denominador);

    if (Inf(g2) == 0){
        SetInf(g2, 0.01);
    }

    interval theta2 = _arg(denominador);

    interval g = g1 / g2;
    interval theta = theta1 - theta2;

#ifdef ARG

    a = cinterval(g * cos (theta), g * sin (theta));

    g = abs(a);
    theta = _arg(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

#else

    if (Inf(theta) > 0){
        theta = interval (Inf(theta) - dPI, Sup(theta) - dPI);
    } else if (Sup(theta) > 0){

        real a = Sup(theta) - 2 * PI;
        real b = Inf(theta);

        if (a > b){
            theta = interval(b, a);
        } else {
            theta = interval(a, b);
        }
    }

#endif

    if (nyquist){

        boxInf = _double(Inf(theta));

        boxDB = cinterval (20.0 * log10(g), theta * 180.0 / PI);

        return cinterval(g * cos (theta), g * sin (theta));
    }


    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

qreal Natura_Interval_extension::getBoxInf(){
    return boxInf;
}

cinterval Natura_Interval_extension::getBoxDB(){
    return boxDB;
}

cinterval Natura_Interval_extension::get_box_nume(QVector <Parameter * > * nume, qreal w, LtiSystem::SystemType tipo, bool nyquist){
    cinterval a;

    if (tipo == LtiSystem::SystemType::PolynomialForm){ //TODO arreglar falta formato libre

        //a = get_box_cpolinomios(sistema, w);

    } else if (tipo == LtiSystem::SystemType::ZeroPoleGain){
        a = get_box_kganancia_nume(nume, w);

    } else {
        //a =  get_box_knoganacia(sistema, w);
    }

    if (nyquist)
        return a;

    interval b = abs(a);

    if (Inf(b) == 0){
        SetInf(b, 0.01);
    }

    return cinterval (20.0 * log10(b), _arg(a) * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box_deno(QVector <Parameter * > * deno, qreal w, LtiSystem::SystemType tipo, bool nyquist){
    cinterval a;

    if (tipo == LtiSystem::SystemType::PolynomialForm){ //TODO arreglar falta formato libre

        //a = get_box_cpolinomios(sistema, w);

    } else if (tipo == LtiSystem::SystemType::ZeroPoleGain){
        a = get_box_kganancia_deno(deno, w);

    } else {
        //a =  get_box_knoganacia(sistema, w);
    }

    if (nyquist)
        return a;

    interval b = abs(a);

    if (Inf(b) == 0){
        SetInf(b, 0.01);
    }

    return cinterval (20.0 * log10(b), _arg(a) * 180.0 / PI);
}

inline cinterval Natura_Interval_extension::get_box_kganancia(LtiSystem *sistema, qreal w){

    QVector <Parameter*> * nume = sistema->numerator();
    QVector <Parameter * > * deno = sistema->denominator();

    Parameter * kv = sistema->gain();


    //Creamos el numerador.
    //Creamos el numerador.
    cinterval numerador (1);
    complex complejo (0, w);


    if (!nume->isEmpty()){
        QVector <Parameter*>::iterator it = nume->begin();

        if ((*it)->isUncertain()){
            numerador = (complejo + interval((*it)->range().x(), (*it)->range().y()));
        }else{
            numerador = (complejo + interval((*it)->nominal()));
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isUncertain()){
                numerador = numerador * (complejo + interval((*it)->range().x(), (*it)->range().y()));
            }else{
                numerador = numerador * (complejo + interval((*it)->nominal()));
            }
        }

    }

    //Creamos el denominador.
    cinterval denominador (interval (1));

    if (!deno->isEmpty()){
        QVector <Parameter*>::iterator it = deno->begin();

        if ((*it)->isUncertain()){
            denominador = (complejo + interval((*it)->range().x(), (*it)->range().y()));
        }else{
            denominador = (complejo + interval((*it)->nominal()));
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isUncertain()){
                denominador = denominador * (complejo + interval((*it)->range().x(), (*it)->range().y()));
            }else{
                denominador = denominador * (complejo + interval((*it)->nominal()));
            }
        }
    }
    

    if(kv->isUncertain()){

        if (0.0 <= Re(denominador) && 0.0 <= Im(denominador) ) {
            cinterval a = interval(kv->range().x(), kv->range().y()) * numerador;
            interval g1 = abs(a);

            if (Inf(g1) == 0){
                SetInf(g1, 0.000001);
            }

            interval theta1 = arg(a);

            interval g2 = abs(denominador);

            if (Inf(g2) == 0){
                SetInf(g2, 0.000001);
            }

            interval theta2 = arg(denominador);

            interval g = g1 / g2;
            interval theta = theta1 - theta2;

            cinterval L (g * cos (theta), g * sin (theta));

            return L;

        } else {
            return interval(kv->range().x(), kv->range().y()) * numerador / denominador;
        }
    } else{
        return kv->nominal() * numerador / denominador;
    }
}

inline cinterval Natura_Interval_extension::get_box_kganancia_nume (QVector <Parameter*> * nume, qreal w){

    cinterval numerador (1);
    complex complejo (0, w);


    if (!nume->isEmpty()){
        QVector <Parameter*>::iterator it = nume->begin();

        if ((*it)->isUncertain()){
            numerador = (complejo + interval((*it)->range().x(), (*it)->range().y()));
        }else{
            numerador = (complejo + interval((*it)->nominal()));
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isUncertain()){
                numerador = numerador * (complejo + interval((*it)->range().x(), (*it)->range().y()));
            }else{
                numerador = numerador * (complejo + interval((*it)->nominal()));
            }
        }
    }

    return numerador;
}

inline cinterval Natura_Interval_extension::get_box_kganancia_deno (QVector <Parameter * > * deno, qreal w){

    cinterval denominador (interval (1));
    complex complejo (0, w);

    if (!deno->isEmpty()){
        QVector <Parameter*>::iterator it = deno->begin();

        if ((*it)->isUncertain()){
            denominador = (complejo + interval((*it)->range().x(), (*it)->range().y()));
        }else{
            denominador = (complejo + interval((*it)->nominal()));
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isUncertain()){
                denominador = denominador * (complejo + interval((*it)->range().x(), (*it)->range().y()));
            }else{
                denominador = denominador * (complejo + interval((*it)->nominal()));
            }
        }
    }
    
    return denominador;
}

inline cinterval Natura_Interval_extension::get_box_knoganacia(LtiSystem *sistema, qreal w){

    QVector <Parameter*> * nume = sistema->numerator();
    QVector <Parameter*> * deno = sistema->denominator();

    Parameter * kv = sistema->gain();

    complex complejo(0, w);

    //Creamos el numerador.

    cinterval numerador(1);

    QVector <Parameter*>::iterator it = nume->begin();

    if (!nume->empty()){

        if ((*it)->isUncertain()){
            numerador = ((complejo / interval((*it)->range().x(),(*it)->range().y())) + 1.);
        }else{
            numerador = ((complejo / (*it)->nominal()) + 1.);
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isUncertain()){
                numerador = numerador * ((complejo / interval((*it)->range().x(),(*it)->range().y())) + 1.);
            }else{
                numerador = numerador * ((complejo / (*it)->nominal()) + 1.);
            }
        }
    }


    cinterval denominador(1);

    if (!deno->empty()){
        it = deno->begin();

        if ((*it)->isUncertain()){
            denominador = ((complejo / interval((*it)->range().x(),(*it)->range().y())) + 1.);
        }else{
            denominador = ((complejo / (*it)->nominal()) + 1.);
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isUncertain()){
                denominador = denominador * ((complejo / interval((*it)->range().x(),(*it)->range().y())) + 1.);
            }else{
                denominador = denominador * ((complejo / (*it)->nominal()) + 1.);
            }
        }
    }


    if(kv->isUncertain()){
        return interval(kv->range().x(), kv->range().y()) * (numerador / denominador);
    } else{
        return kv->nominal() * (numerador / denominador);
    }
}

inline cinterval Natura_Interval_extension::get_box_cpolinomios(LtiSystem * sistema, qreal w){

    QVector <Parameter*> * nume = sistema->numerator();
    QVector <Parameter * > * deno = sistema->denominator();

    Parameter * kv = sistema->gain();

    //Calculamos el numerador
    qint32 expo = nume->size() - 1;
    cinterval numerador (0);
    complex complejo (0, w);
    
    for (qint32 i = 1;  i < nume->size(); i++) {
        Parameter * n = nume->at(i);
        if (n->isUncertain()) {
            QPointF x = n->range();
            numerador = numerador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
        } else {
            numerador = numerador + pow(n->nominal() * complejo, expo);
        }
        expo--;
    }

    Parameter * n = nume->last();

    if (n->isUncertain()){
        QPointF x = n->range();
        numerador = numerador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
    }else {
        numerador = numerador + pow(n->nominal() * complejo, expo);
    }


    //Calculamos el denominador
    expo = deno->size() - 1;
    cinterval denominador (0);

    for (qint32 i = 1;  i < deno->size(); i++) {
        Parameter * n = deno->at(i);
        if (n->isUncertain()) {
            QPointF x = n->range();
            denominador = denominador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
        } else {
            denominador = denominador + pow(n->nominal() * complejo, expo);
        }
        expo--;
    }

    n = deno->last();

    if (n->isUncertain()){
        QPointF x = n->range();
        denominador = denominador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
    }else {
        denominador = denominador + pow(n->nominal() * complejo, expo);
    }


    if(kv->isUncertain()){
        return interval(kv->range().x(), kv->range().y()) * (numerador / denominador);
    } else{
        return kv->nominal() * (numerador / denominador);
    }

}

inline interval Natura_Interval_extension::_arg(cinterval z)
{
    //return  cxsc::arg(z) - PI;

    qreal
            r0 = _double(InfRe(z)),
            r1 = _double(SupRe(z)),

            i0 = _double(InfIm(z)),
            i1 = _double(SupIm(z));


    qreal dospi = 2 * PI;

    qreal a,b;

    if (r0 >= 0 && r1 >= 0 && i0 >= 0 && i1 >= 0){ //1
        a = std::atan2(i0,r1) - dospi, b = std::atan2(i1,r0) - dospi;
    } else if (r0 >= 0 && r1 >= 0 && i0 <= 0 && i1 >= 0){ // 2
        a = std::atan2(i1, r0) - dospi, b = std::atan2(i0, r0);
    } else if (i0 <= 0 && i1 <= 0 && r0 >= 0 && r1 >= 0){ //3
        a = std::atan2(i0,r0), b = std::atan2(i1,r1);
    } else if (i0 <= 0 && i1 <= 0 && r0 <= 0 && r1 >= 0){ // 4
        a = std::atan2(i1, r0), b = std::atan2(i1, r1);
    } else if (i0 <= 0 && i1 <= 0 && r0 <= 0 && r1 <= 0){ //5
        a = std::atan2(i1,r0), b = std::atan2(i0,r1);
    } else if(r0 <= 0 && r1 <= 0 && i0 <= 0 && i1 >= 0){ // 6
        a = std::atan2(i1,r1) - dospi, b = std::atan2(i0, r1);
    } else if (r0 <= 0 && r1 <= 0 && i0 >= 0 && i1 >= 0){ //7
        a = std::atan2(i1,r1) - dospi,b = std::atan2(i0,r0) - dospi;
    } else if(i0 >= 0 && i1 >= 0 && r0 <= 0 && r1 >= 0){ // 8
        a = std::atan2(i0,r1) - dospi, b = std::atan2(i0,r0) - dospi;
    } else{
        return interval  (-dospi, 0);
    }

    return interval (a, b);

}
