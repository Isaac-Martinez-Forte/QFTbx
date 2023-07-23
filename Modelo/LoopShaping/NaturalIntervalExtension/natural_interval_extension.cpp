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

cinterval Natura_Interval_extension::get_box_termino_nume(Var * var, qreal w, complex p0) {

    cinterval a = (complex (0, w) + interval(var->getRango().x(), var->getRango().y())) * p0;

    interval g = abs(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

    interval theta = _arg(a);

    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box_termino_deno(Var * var, qreal w, complex p0) {

    cinterval a = 1 / (complex (0, w) + interval(var->getRango().x(), var->getRango().y())) * p0;

    interval g = abs(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

    interval theta = _arg(a);

    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box_termino_k(Var * var, complex p0) {

    cinterval a = (interval(var->getRango().x(), var->getRango().y())) * p0;

    interval g = abs(a);

    if (Inf(g) == 0){
        SetInf(g, 0.01);
    }

    interval theta = _arg(a);

    return cinterval (20.0 * log10(g), theta * 180.0 / PI);
}

cinterval Natura_Interval_extension::get_box(std::shared_ptr<Sistema> sistema, qreal w, complex p0, bool nyquist){

    cinterval a;

    /*if (sistema->getClass() == Sistema::cof_polinomios){ //TODO arreglar falta formato libre

        a = get_box_cpolinomios(sistema, w);

    } else if (sistema->getClass() == Sistema::k_ganancia){
        a = get_box_kganancia(sistema, w);

    } else {
        a =  get_box_knoganacia(sistema, w);
    }*/

    //////////////////////////////////////////////////////////////////

    QVector <Var*> * nume = sistema->getNumerador();
    QVector <Var * > * deno = sistema->getDenominador();

    Var * kv = sistema->getK();


    //Creamos el numerador.
    cinterval numerador;
    complex complejo (0, w);


    if (!nume->isEmpty()){
        QVector <Var*>::iterator it = nume->begin();

        if ((*it)->isVariable()){
            numerador = (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
        }else{
            numerador = (complejo + interval((*it)->getNominal()));
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isVariable()){
                numerador = numerador * (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
            }else{
                numerador = numerador * (complejo + interval((*it)->getNominal()));
            }
        }

    }

    //Creamos el denominador.
    cinterval denominador;

    if (!deno->isEmpty()){
        QVector <Var*>::iterator it = deno->begin();

        if ((*it)->isVariable()){
            denominador = (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
        }else{
            denominador = (complejo + interval((*it)->getNominal()));
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isVariable()){
                denominador = denominador * (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
            }else{
                denominador = denominador * (complejo + interval((*it)->getNominal()));
            }
        }
    }


    if(kv->isVariable()){
        a = interval(kv->getRango().x(), kv->getRango().y()) * numerador * p0;
    } else{
        a = kv->getNominal() * numerador * p0;
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

cinterval Natura_Interval_extension::get_box_nume(QVector <Var * > * nume, qreal w, Sistema::tipo_planta tipo, bool nyquist){
    cinterval a;

    if (tipo == Sistema::cof_polinomios){ //TODO arreglar falta formato libre

        //a = get_box_cpolinomios(sistema, w);

    } else if (tipo == Sistema::k_ganancia){
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

cinterval Natura_Interval_extension::get_box_deno(QVector <Var * > * deno, qreal w, Sistema::tipo_planta tipo, bool nyquist){
    cinterval a;

    if (tipo == Sistema::cof_polinomios){ //TODO arreglar falta formato libre

        //a = get_box_cpolinomios(sistema, w);

    } else if (tipo == Sistema::k_ganancia){
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

inline cinterval Natura_Interval_extension::get_box_kganancia(std::shared_ptr<Sistema> sistema, qreal w){

    QVector <Var*> * nume = sistema->getNumerador();
    QVector <Var * > * deno = sistema->getDenominador();

    Var * kv = sistema->getK();


    //Creamos el numerador.
    //Creamos el numerador.
    cinterval numerador (1);
    complex complejo (0, w);


    if (!nume->isEmpty()){
        QVector <Var*>::iterator it = nume->begin();

        if ((*it)->isVariable()){
            numerador = (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
        }else{
            numerador = (complejo + interval((*it)->getNominal()));
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isVariable()){
                numerador = numerador * (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
            }else{
                numerador = numerador * (complejo + interval((*it)->getNominal()));
            }
        }

    }

    //Creamos el denominador.
    cinterval denominador (interval (1));

    if (!deno->isEmpty()){
        QVector <Var*>::iterator it = deno->begin();

        if ((*it)->isVariable()){
            denominador = (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
        }else{
            denominador = (complejo + interval((*it)->getNominal()));
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isVariable()){
                denominador = denominador * (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
            }else{
                denominador = denominador * (complejo + interval((*it)->getNominal()));
            }
        }
    }
    

    if(kv->isVariable()){

        if (0.0 <= Re(denominador) && 0.0 <= Im(denominador) ) {
            cinterval a = interval(kv->getRango().x(), kv->getRango().y()) * numerador;
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
            return interval(kv->getRango().x(), kv->getRango().y()) * numerador / denominador;
        }
    } else{
        return kv->getNominal() * numerador / denominador;
    }
}

inline cinterval Natura_Interval_extension::get_box_kganancia_nume (QVector <Var*> * nume, qreal w){

    cinterval numerador (1);
    complex complejo (0, w);


    if (!nume->isEmpty()){
        QVector <Var*>::iterator it = nume->begin();

        if ((*it)->isVariable()){
            numerador = (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
        }else{
            numerador = (complejo + interval((*it)->getNominal()));
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isVariable()){
                numerador = numerador * (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
            }else{
                numerador = numerador * (complejo + interval((*it)->getNominal()));
            }
        }
    }

    return numerador;
}

inline cinterval Natura_Interval_extension::get_box_kganancia_deno (QVector <Var * > * deno, qreal w){

    cinterval denominador (interval (1));
    complex complejo (0, w);

    if (!deno->isEmpty()){
        QVector <Var*>::iterator it = deno->begin();

        if ((*it)->isVariable()){
            denominador = (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
        }else{
            denominador = (complejo + interval((*it)->getNominal()));
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isVariable()){
                denominador = denominador * (complejo + interval((*it)->getRango().x(), (*it)->getRango().y()));
            }else{
                denominador = denominador * (complejo + interval((*it)->getNominal()));
            }
        }
    }
    
    return denominador;
}

inline cinterval Natura_Interval_extension::get_box_knoganacia(std::shared_ptr<Sistema> sistema, qreal w){

    QVector <Var*> * nume = sistema->getNumerador();
    QVector <Var*> * deno = sistema->getDenominador();

    Var * kv = sistema->getK();

    complex complejo(0, w);

    //Creamos el numerador.

    cinterval numerador(1);

    QVector <Var*>::iterator it = nume->begin();

    if (!nume->empty()){

        if ((*it)->isVariable()){
            numerador = ((complejo / interval((*it)->getRango().x(),(*it)->getRango().y())) + 1.);
        }else{
            numerador = ((complejo / (*it)->getNominal()) + 1.);
        }

        it++;

        for (; it != nume->end(); it++) {
            if ((*it)->isVariable()){
                numerador = numerador * ((complejo / interval((*it)->getRango().x(),(*it)->getRango().y())) + 1.);
            }else{
                numerador = numerador * ((complejo / (*it)->getNominal()) + 1.);
            }
        }
    }


    cinterval denominador(1);

    if (!deno->empty()){
        it = deno->begin();

        if ((*it)->isVariable()){
            denominador = ((complejo / interval((*it)->getRango().x(),(*it)->getRango().y())) + 1.);
        }else{
            denominador = ((complejo / (*it)->getNominal()) + 1.);
        }

        it++;

        for (; it != deno->end(); it++) {
            if ((*it)->isVariable()){
                denominador = denominador * ((complejo / interval((*it)->getRango().x(),(*it)->getRango().y())) + 1.);
            }else{
                denominador = denominador * ((complejo / (*it)->getNominal()) + 1.);
            }
        }
    }


    if(kv->isVariable()){
        return interval(kv->getRango().x(), kv->getRango().y()) * (numerador / denominador);
    } else{
        return kv->getNominal() * (numerador / denominador);
    }
}

inline cinterval Natura_Interval_extension::get_box_cpolinomios(std::shared_ptr<Sistema> sistema, qreal w){

    QVector <Var*> * nume = sistema->getNumerador();
    QVector <Var * > * deno = sistema->getDenominador();

    Var * kv = sistema->getK();

    //Calculamos el numerador
    qint32 expo = nume->size() - 1;
    cinterval numerador (0);
    complex complejo (0, w);
    
    for (qint32 i = 1;  i < nume->size(); i++) {
        Var * n = nume->at(i);
        if (n->isVariable()) {
            QPointF x = n->getRango();
            numerador = numerador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
        } else {
            numerador = numerador + pow(n->getNominal() * complejo, expo);
        }
        expo--;
    }

    Var * n = nume->last();

    if (n->isVariable()){
        QPointF x = n->getRango();
        numerador = numerador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
    }else {
        numerador = numerador + pow(n->getNominal() * complejo, expo);
    }


    //Calculamos el denominador
    expo = deno->size() - 1;
    cinterval denominador (0);

    for (qint32 i = 1;  i < deno->size(); i++) {
        Var * n = deno->at(i);
        if (n->isVariable()) {
            QPointF x = n->getRango();
            denominador = denominador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
        } else {
            denominador = denominador + pow(n->getNominal() * complejo, expo);
        }
        expo--;
    }

    n = deno->last();

    if (n->isVariable()){
        QPointF x = n->getRango();
        denominador = denominador + pow(interval(x.x(), x.y()) * complejo, interval(expo));
    }else {
        denominador = denominador + pow(n->getNominal() * complejo, expo);
    }


    if(kv->isVariable()){
        return interval(kv->getRango().x(), kv->getRango().y()) * (numerador / denominador);
    } else{
        return kv->getNominal() * (numerador / denominador);
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
