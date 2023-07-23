#include "knganancia.h"

using namespace std;

KNGanancia::KNGanancia(QString nombre, QVector<Var *> *numerador, QVector<Var *> *denominador, Var *k, Var *ret):
    PolosCeros(nombre, numerador, denominador,k,ret)
{
}

KNGanancia::~KNGanancia(){
}

std::shared_ptr<Sistema> KNGanancia::invoke(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret, QString exp_nume __attribute__((unused)), QString exp_deno __attribute__((unused))){
    return std::make_shared<KNGanancia>(nombre, numerador, denominador, k, ret);
}

QString KNGanancia::getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
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


    for (qint32 i = 1; i < sizeDen; i++){

        es += "((("+ QString::number(omega) + "*i) / " + QString::number(denominador->at(i)) + ")+1) *";
    }

    if (sizeDen == 0){
        es += "(1))";
    }else {
        es += "((("+ QString::number(omega) + "*i) /" + QString::number(denominador->last()) + ")+1))";
    }


    if (ret != 0){
        es += "* e^(i*" + QString::number(omega) + "*" + QString::number(ret) +")";
    }


    return es;
}

QString KNGanancia::getExpr(qreal w){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isVariable()){
        es += "kv*(";
    }else {
        es += QString::number(k->getNominal()) + "*(";
    }


    for (qint32 i = 0; i < sizeNum-1; i++){

        if (numerador->at(i)->isVariable()){
            es += "(((" + QString::number(w) + "*i) / " + numerador->at(i)->getNombre() + ")+1) *";
        } else {
            es += "((("+ QString::number(w) + "*i) /" + QString::number(numerador->at(i)->getNominal()) + ")+1) *";
        }
    }

    if (sizeNum == 0){
        es += "(1)) / (";
    }else {
        if(numerador->last()->isVariable()){
            es += "(((" + QString::number(w) + "*i) / " + numerador->last()->getNombre() + ")+1)) / (";
        } else {
            es += "(((" + QString::number(w) + "*i) / " + QString::number(numerador->last()->getNominal()) + ")+1)) / (";
        }
    }

    for (qint32 i = 0; i < sizeDen-1; i++){

        if (denominador->at(i)->isVariable()){
            es += "(((" + QString::number(w) + "*i) / " + denominador->at(i)->getNombre() + ")+1) *";
        } else {
            es += "((("+ QString::number(w) + "*i) / " + QString::number(denominador->at(i)->getNominal()) + ")+1) *";
        }
    }

    if (sizeDen == 0){
        es += "(1))";
    } else {

        if (denominador->last()->isVariable()){
            es += "(((" + QString::number(w) + "*i) / " + denominador->last()->getNombre() + ")+1))";
        }else{
            es += "((("+ QString::number(w) + "*i) /" + QString::number(denominador->last()->getNominal()) + ")+1))";
        }
    }

    if (ret->getNominal() != 0){
        if (ret->isVariable()){
            es += "* e^(i*" + QString::number(w) + "*ret)";
        }else{
            es += "* e^(i*" + QString::number(w) + "*" + QString::number(ret->getNominal()) +")";
        }
    }

    return es;
}

QString KNGanancia::getExpr(){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isVariable()){
        es += "kv*(";
    }else {
        es += QString::number(k->getNominal()) + "*(";
    }


    for (qint32 i = 0; i < sizeNum-1; i++){

        if (numerador->at(i)->isVariable()){
            es += "(s / " + numerador->at(i)->getNombre() + "+1) *";
        } else {
            es += "(s /" + QString::number(numerador->at(i)->getNominal()) + "+1) *";
        }
    }

    if (sizeNum == 0){
        es += "(1)) / (";
    }else {

        if(numerador->last()->isVariable()){
            es += "(s / " + numerador->last()->getNombre() + "+1)) / (";
        } else {
            es += "(s / " + QString::number(numerador->last()->getNominal()) + "+1)) / (";
        }
    }

    for (qint32 i = 0; i < sizeDen-1; i++){

        if (denominador->at(i)->isVariable()){
            es += "(s / " + denominador->at(i)->getNombre() + "+1) *";
        } else {
            es += "(s / " + QString::number(denominador->at(i)->getNominal()) + "+1) *";
        }
    }

    if (sizeDen == 0){
        es += "(1))";
    } else {

        if (denominador->last()->isVariable()){
            es += "(s / " + denominador->last()->getNombre() + "+1))";
        }else{
            es += "(s /" + QString::number(denominador->last()->getNominal()) + "+1))";
        }
    }

    if (ret->getNominal() != 0){
        if (ret->isVariable()){
            es += "* e^(s*ret)";
        }else{
            es += "* e^(s*" + QString::number(ret->getNominal()) +")";
        }
    }

    return es;
}

Sistema::tipo_planta KNGanancia::getClass(){
    return k_no_ganancia;
}



std::complex <qreal> KNGanancia::getPuntoNume(QVector <qreal> * nume, qreal omega){


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

std::complex <qreal> KNGanancia::getPuntoDeno(QVector <qreal> * deno, qreal omega){

    if (deno->isEmpty()){
        return std::complex <qreal> (1);
    }

    qint32 sizeDen = deno->size();
    QString es = "(";

    for (qint32 i = 1; i < sizeDen; i++){

        es += "((("+ QString::number(omega) + "*i) / " + QString::number(deno->at(i)) + ")+1) *";
    }

    es += "((("+ QString::number(omega) + "*i) /" + QString::number(deno->last()) + ")+1))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}
