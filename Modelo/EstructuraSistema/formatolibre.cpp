#include "formatolibre.h"

using namespace std;
using namespace mup;

FormatoLibre::FormatoLibre(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k,
                           Var* ret, QString exp_nume, QString exp_deno)
    :FuncionTransferencia (nombre, numerador, denominador, k, ret)
{
    this->exp_nume = exp_nume;
    this->exp_deno = exp_deno;
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

QString FormatoLibre::getExpr(qreal w){

    QString n = exp_nume;
    QString d = exp_deno;

    QString es = k->getExp() + "*(" + n.replace("s", "(" + QString::number(w) + "*i)") + ")/(" +
            d.replace("s", "(" + QString::number(w) + "*i)") + ")";


    //El retardo puro es e^(-s*tau) => e^(-i*w*tau). Se emite si es variable
    //(aunque su nominal sea 0, para que el barrido de templates lo recorra)
    //o si es una constante no nula.
    if (ret->isVariable()){
        es += "* e^(-i*" + QString::number(w) + "*" + ret->getNombre() + ")";
    }else if (ret->getNominal() != 0){
        es += "* e^(-i*" + QString::number(w) + "*" +
                QString::number(ret->getNominal()) +")";
    }

    return es;
}

QString FormatoLibre::getExpr(){
    QString es = k->getExp() + "*(" + exp_nume + ")/(" + exp_deno + ")";

    if (ret->isVariable()){
        es += " * e^(-s*" + ret->getNombre() + ")";
    }else if (ret->getNominal() != 0){
        es += " * e^(-s*" + QString::number(ret->getNominal()) +")";
    }

    return es;
}

Sistema::tipo_planta FormatoLibre::getClass(){
    return formato_libre;
}

Sistema * FormatoLibre::invoke(QString nombre, QVector<Var *> *numerador, QVector<Var *> *denominador,
                               Var *k, Var *ret, QString exp_nume, QString exp_deno){

    //Un retardo no especificado equivale a retardo cero.
    return new FormatoLibre (nombre, numerador, denominador, k,
                             ret == NULL ? new Var(0.0) : ret, exp_nume, exp_deno);
}


QString FormatoLibre::getNumeradorString(){
    return exp_nume;
}

QString FormatoLibre::getDenominadorString(){
    return exp_deno;
}

Sistema * FormatoLibre::clone(){

    Var * k = this->k->clone();
    Var * ret = this->ret->clone();

    return this->invoke(this->getNombre(), Var::clonarVector(numerador),
                        Var::clonarVector(denominador), k, ret,
                        this->exp_nume, this->exp_deno);
}
