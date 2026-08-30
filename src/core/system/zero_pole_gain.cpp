#include "zero_pole_gain.h"

using namespace std;
using namespace mup;

ZeroPoleGain::ZeroPoleGain(QString nombre, QVector<Parameter *> *numerador, QVector<Parameter *> *denominador, Parameter *k, Parameter *ret):
    TransferFunction(nombre, numerador, denominador,k,ret)
{
}

ZeroPoleGain::~ZeroPoleGain(){
}

LtiSystem * ZeroPoleGain::create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                             Parameter * k, Parameter* ret, QString exp_nume __attribute__((unused)), QString exp_deno __attribute__((unused))){
    //Un retardo no especificado equivale a retardo cero.
    return new ZeroPoleGain(nombre, numerador, denominador, k, ret == NULL ? new Parameter(0.0) : ret);
}



QString ZeroPoleGain::expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                            qreal k, qreal ret, qreal omega){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;


    es += QString::number(k) + "*(";


    if (numerador->isEmpty()){
        es += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            es += "(("+ QString::number(omega) + "*i) +" + QString::number(numerador->at(i)) + ") *";
        }

        es += "((" + QString::number(omega) + "*i) + " + QString::number(numerador->last()) + ")) / (";
    }


    if (denominador->isEmpty()){
        es += "1)";
    } else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            es += "(("+ QString::number(omega) + "*i) + " + QString::number(denominador->at(i)) + ") *";
        }

        es += "(("+ QString::number(omega) + "*i) + " + QString::number(denominador->last()) + "))";
    }

    if (ret != 0){
        es += "* e^(-i*" + QString::number(omega) + "*" + QString::number(ret) +")";
    }


    return es;
}

QString ZeroPoleGain::expression(qreal w){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isUncertain()){
        es += k->name() + "*(";
    }else {
        es += QString::number(k->nominal()) + "*(";
    }

    if (numerador->isEmpty()){
        es += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            if (numerador->at(i)->isUncertain()){
                es += "((" + QString::number(w) + "*i) + " + numerador->at(i)->name() + ") *";
            } else {
                es += "(("+ QString::number(w) + "*i) +" + QString::number(numerador->at(i)->nominal()) + ") *";
            }
        }

        if(numerador->last()->isUncertain()){
            es += "((" + QString::number(w) + "*i) + " + numerador->last()->name() + ")) / (";
        } else {
            es += "((" + QString::number(w) + "*i) + " + QString::number(numerador->last()->nominal()) + ")) / (";
        }
    }


    if (denominador->isEmpty()){
        es += "1)";
    } else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            if (denominador->at(i)->isUncertain()){
                es += "((" + QString::number(w) + "*i) + " + denominador->at(i)->name() + ") *";
            } else {
                es += "(("+ QString::number(w) + "*i) + " + QString::number(denominador->at(i)->nominal()) + ") *";
            }
        }


        if (denominador->last()->isUncertain()){
            es += "((" + QString::number(w) + "*i) + " + denominador->last()->name() + "))";
        }else{
            es += "(("+ QString::number(w) + "*i) + " + QString::number(denominador->last()->nominal()) + "))";
        }
    }


    //El retardo puro es e^(-s*tau) => e^(-i*w*tau). Se emite si el retardo es
    //variable (aunque su nominal sea 0, el barrido de templates lo recorre) o
    //si es una constante no nula.
    if (ret->isUncertain()){
        es += "* e^(-i*" + QString::number(w) + "*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += "* e^(-i*" + QString::number(w) + "*" + QString::number(ret->nominal()) +")";
    }

    return es;
}


LtiSystem::SystemType ZeroPoleGain::type(){
    return SystemType::ZeroPoleGain;
}


QString ZeroPoleGain::expression(){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isUncertain()){
        es += k->name() + "*(";
    }else {
        es += QString::number(k->nominal()) + "*(";
    }

    if (numerador->isEmpty()){
        es += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            if (numerador->at(i)->isUncertain()){
                es += "(s + " + numerador->at(i)->name() + ") *";
            } else {
                es += "(s +" + QString::number(numerador->at(i)->nominal()) + ") *";
            }
        }

        if(numerador->last()->isUncertain()){
            es += "(s + " + numerador->last()->name() + ")) / (";
        } else {
            es += "(s + " + QString::number(numerador->last()->nominal()) + ")) / (";
        }
    }


    if (denominador->isEmpty()){
        es += "1)";
    }else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            if (denominador->at(i)->isUncertain()){
                es += "(s + " + denominador->at(i)->name() + ") *";
            } else {
                es += "(s + " + QString::number(denominador->at(i)->nominal()) + ") *";
            }
        }

        if (denominador->last()->isUncertain()){
            es += "(s + " + denominador->last()->name() + "))";
        }else{
            es += "(s + " + QString::number(denominador->last()->nominal()) + "))";
        }
    }

    if (ret->isUncertain()){
        es += " * e^(-s*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += " * e^(-s*" + QString::number(ret->nominal()) +")";
    }

    return es;
}


std::complex <qreal> ZeroPoleGain::evaluateNumerator(QVector <qreal> * nume, qreal omega){

    if (nume->isEmpty()){
        return std::complex <qreal>(1);
    }

    qint32 sizeNum = nume->size();
    QString es = "(";

    for (qint32 i = 0; i < sizeNum-1; i++){

        es += "(("+ QString::number(omega) + "*i) +" + QString::number(nume->at(i)) + ") *";
    }

    es += "((" + QString::number(omega) + "*i) + " + QString::number(nume->last()) + "))";


    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> ZeroPoleGain::evaluateDenominator(QVector <qreal> * deno, qreal omega){

    if (deno->isEmpty()){
        return std::complex <qreal>(1);
    }

    qint32 sizeDen = deno->size();
    QString es = "(";

    for (qint32 i = 0; i < sizeDen-1; i++){

        es += "(("+ QString::number(omega) + "*i) + " + QString::number(deno->at(i)) + ") *";
    }

    es += "(("+ QString::number(omega) + "*i) + " + QString::number(deno->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}
