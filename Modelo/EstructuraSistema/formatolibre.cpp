#include "formatolibre.h"

using namespace std;
using namespace mup;

FormatoLibre::FormatoLibre(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k,
                           Var* ret, QString exp_nume, QString exp_deno)
    :Sistema (nombre)
{
    b = true;

    this->numerador = numerador;
    this->denominador = denominador;
    this->k = k;
    this->ret = ret;
    this->exp_nume = exp_nume;
    this->exp_deno = exp_deno;
}

FormatoLibre::FormatoLibre(FormatoLibre* datos) : FormatoLibre (datos->getNombre(), datos->getNumerador(), datos->getDenominador(),
                                                                datos->getK(), datos->getRet(), datos->exp_nume, datos->exp_nume) {

}

FormatoLibre::~FormatoLibre(){

    if (b){
        numerador->clear();
        denominador->clear();
        delete k;
        delete ret;
    }
}

void FormatoLibre::noBorrar(){
    b = false;
}

std::complex <qreal> FormatoLibre::getPunto (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                             qreal k, qreal ret, qreal omega){ //TODO ver que hacer con esto
    return complex <qreal> ();
}

QString FormatoLibre::getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                               qreal k, qreal ret, qreal omega){//TODO ver que hacer con esto
    return "";
}

std::complex <qreal> FormatoLibre::getPuntoNume(QVector <qreal> * nume, qreal omega){


    return complex <qreal> ();
}

std::complex <qreal> FormatoLibre::getPuntoDeno(QVector <qreal> * deno, qreal omega){


    return complex <qreal> ();
}

complex<qreal> FormatoLibre::getPunto(qreal w){
    ParserX p (pckALL_COMPLEX);

    p.EnableAutoCreateVar(true);

    QString es;

    foreach (Var * n, *numerador) {

        if (n->isVariable()){
            es = n->getNombre() + "=" + QString::number(n->getNominal());
            p.SetExpr(es.toStdString());
            p.Eval();
        }
    }

    foreach (Var * d, *denominador) {
        if (d->isVariable()){
            es = d->getNombre() + "=" + QString::number(d->getNominal());
            p.SetExpr(es.toStdString());
            p.Eval();
        }
    }

    if (k->isVariable()){
        es = k->getNombre() + "=" + QString::number(k->getNominal());
        p.SetExpr(es.toStdString());
        p.Eval();
    }

    if (ret->isVariable()){
        es = ret->getNombre() + "=" + QString::number(ret->getNominal());
        p.SetExpr(es.toStdString());
        p.Eval();
    }

    es = getExpr(w);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}

QString FormatoLibre::getExpr(qreal w){

    QString n = exp_nume;
    QString d = exp_deno;

    QString es = k->getExp() + "*(" + n.replace("s", "(" + QString::number(w) + "*i)") + ")/(" +
            d.replace("s", "(" + QString::number(w) + "*i)") + ")";


    if (ret->getNominal() != 0){
        es += "* e^(i*" + QString::number(w) + "*" +
                QString::number(ret->getNominal()) +")";
    }

    return es;
}

QString FormatoLibre::getExpr(){
    return k->getExp() + "*(" + exp_nume + ")/(" + exp_deno + ")";
}

Sistema::tipo_planta FormatoLibre::getClass(){
    return formato_libre;
}

QVector <std::complex <qreal> > * FormatoLibre::getPunto (QVector <qreal> * omega){
    QVector <complex <qreal> > * r = new QVector <complex <qreal> > ();

    foreach (qreal o, *omega) {
        r->append(getPunto(o));
    }

    return r;
}

QVector <Var*> * FormatoLibre::getDenominador(){
    return denominador;
}

QVector <Var*> * FormatoLibre::getNumerador(){
    return numerador;
}

Var * FormatoLibre::getK (){
    return k;
}

Var * FormatoLibre::getRet(){
    return ret;
}

Sistema * FormatoLibre::invoke(QString nombre, QVector<Var *> *numerador, QVector<Var *> *denominador,
                               Var *k, Var *ret, QString exp_nume, QString exp_deno){

    return new FormatoLibre (nombre, numerador, denominador, k, ret, exp_nume, exp_deno);
}


QString FormatoLibre::getNumeradorString(){
    return exp_nume;
}

QString FormatoLibre::getDenominadorString(){
    return exp_deno;
}

Sistema * FormatoLibre::clone(){

    QVector <Var *> * n = new QVector <Var *> ();
    QVector <Var *> * d = new QVector <Var *> ();

    Var * k = this->k->clone();

    Var * ret = this->ret->clone();

    foreach (Var * v, *numerador) {
        n->append(v->clone());
    }

    foreach (Var * v, *denominador) {
        d->append(v->clone());
    }


    return this->invoke(this->getNombre(), n, d, k, ret, this->exp_nume, this->exp_deno);
}

