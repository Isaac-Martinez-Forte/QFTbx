#include "time_constant_gain.h"

using namespace std;

TimeConstantGain::TimeConstantGain(QString nombre, QVector<Parameter *> *numerador, QVector<Parameter *> *denominador, Parameter *k, Parameter *ret):
    TransferFunction(nombre, numerador, denominador,k,ret)
{
}

TimeConstantGain::~TimeConstantGain(){
}

LtiSystem * TimeConstantGain::create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret, QString exp_nume __attribute__((unused)), QString exp_deno __attribute__((unused))){
    //Un retardo no especificado equivale a retardo cero.
    return new TimeConstantGain(nombre, numerador, denominador, k, ret == NULL ? new Parameter(0.0) : ret);
}

QString TimeConstantGain::expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    es += QString::number(k) + "*(";


    for (qint32 i = 0; i < sizeNum-1; i++){

        es += "((("+ QString::number(omega) + "*i) /" + QString::number(numerador->at(i)) + ")+1) *";
    }

    if (sizeNum == 0){
        es += "(1)) / (";
    } else{
        es += "(((" + QString::number(omega) + "*i) / " + QString::number(numerador->last()) + ")+1)) / (";
    }


    for (qint32 i = 0; i < sizeDen-1; i++){

        es += "((("+ QString::number(omega) + "*i) / " + QString::number(denominador->at(i)) + ")+1) *";
    }

    if (sizeDen == 0){
        es += "(1))";
    }else {
        es += "((("+ QString::number(omega) + "*i) /" + QString::number(denominador->last()) + ")+1))";
    }


    if (ret != 0){
        es += "* e^(-i*" + QString::number(omega) + "*" + QString::number(ret) +")";
    }


    return es;
}

QString TimeConstantGain::expression(qreal w){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isUncertain()){
        es += k->name() + "*(";
    }else {
        es += QString::number(k->nominal()) + "*(";
    }


    for (qint32 i = 0; i < sizeNum-1; i++){

        if (numerador->at(i)->isUncertain()){
            es += "(((" + QString::number(w) + "*i) / " + numerador->at(i)->name() + ")+1) *";
        } else {
            es += "((("+ QString::number(w) + "*i) /" + QString::number(numerador->at(i)->nominal()) + ")+1) *";
        }
    }

    if (sizeNum == 0){
        es += "(1)) / (";
    }else {
        if(numerador->last()->isUncertain()){
            es += "(((" + QString::number(w) + "*i) / " + numerador->last()->name() + ")+1)) / (";
        } else {
            es += "(((" + QString::number(w) + "*i) / " + QString::number(numerador->last()->nominal()) + ")+1)) / (";
        }
    }

    for (qint32 i = 0; i < sizeDen-1; i++){

        if (denominador->at(i)->isUncertain()){
            es += "(((" + QString::number(w) + "*i) / " + denominador->at(i)->name() + ")+1) *";
        } else {
            es += "((("+ QString::number(w) + "*i) / " + QString::number(denominador->at(i)->nominal()) + ")+1) *";
        }
    }

    if (sizeDen == 0){
        es += "(1))";
    } else {

        if (denominador->last()->isUncertain()){
            es += "(((" + QString::number(w) + "*i) / " + denominador->last()->name() + ")+1))";
        }else{
            es += "((("+ QString::number(w) + "*i) /" + QString::number(denominador->last()->nominal()) + ")+1))";
        }
    }

    //El retardo puro es e^(-s*tau) => e^(-i*w*tau). Se emite si es variable
    //(aunque su nominal sea 0) o si es una constante no nula. Se usa el
    //nombre real de la variable, no el literal "ret".
    if (ret->isUncertain()){
        es += "* e^(-i*" + QString::number(w) + "*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += "* e^(-i*" + QString::number(w) + "*" + QString::number(ret->nominal()) +")";
    }

    return es;
}

QString TimeConstantGain::expression(){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isUncertain()){
        es += k->name() + "*(";
    }else {
        es += QString::number(k->nominal()) + "*(";
    }


    for (qint32 i = 0; i < sizeNum-1; i++){

        if (numerador->at(i)->isUncertain()){
            es += "(s / " + numerador->at(i)->name() + "+1) *";
        } else {
            es += "(s /" + QString::number(numerador->at(i)->nominal()) + "+1) *";
        }
    }

    if (sizeNum == 0){
        es += "(1)) / (";
    }else {

        if(numerador->last()->isUncertain()){
            es += "(s / " + numerador->last()->name() + "+1)) / (";
        } else {
            es += "(s / " + QString::number(numerador->last()->nominal()) + "+1)) / (";
        }
    }

    for (qint32 i = 0; i < sizeDen-1; i++){

        if (denominador->at(i)->isUncertain()){
            es += "(s / " + denominador->at(i)->name() + "+1) *";
        } else {
            es += "(s / " + QString::number(denominador->at(i)->nominal()) + "+1) *";
        }
    }

    if (sizeDen == 0){
        es += "(1))";
    } else {

        if (denominador->last()->isUncertain()){
            es += "(s / " + denominador->last()->name() + "+1))";
        }else{
            es += "(s /" + QString::number(denominador->last()->nominal()) + "+1))";
        }
    }

    if (ret->isUncertain()){
        es += "* e^(-s*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += "* e^(-s*" + QString::number(ret->nominal()) +")";
    }

    return es;
}

LtiSystem::SystemType TimeConstantGain::type(){
    return SystemType::TimeConstantGain;
}



std::complex <qreal> TimeConstantGain::evaluateNumerator(QVector <qreal> * nume, qreal omega){


    if (nume->isEmpty()){
        return std::complex <qreal> (1);
    }

    qint32 sizeNum = nume->size();
    QString es = "(";


    for (qint32 i = 0; i < sizeNum-1; i++){

        es += "((("+ QString::number(omega) + "*i) /" + QString::number(nume->at(i)) + ")+1) *";
    }

    es += "(((" + QString::number(omega) + "*i) / " + QString::number(nume->last()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> TimeConstantGain::evaluateDenominator(QVector <qreal> * deno, qreal omega){

    if (deno->isEmpty()){
        return std::complex <qreal> (1);
    }

    qint32 sizeDen = deno->size();
    QString es = "(";

    for (qint32 i = 0; i < sizeDen-1; i++){

        es += "((("+ QString::number(omega) + "*i) / " + QString::number(deno->at(i)) + ")+1) *";
    }

    es += "((("+ QString::number(omega) + "*i) /" + QString::number(deno->last()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}
