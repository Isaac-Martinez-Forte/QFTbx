#include "cpolinomios.h"

using namespace std;

CPolinomios::CPolinomios(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret):
    FuncionTransferencia(nombre, numerador, denominador, k , ret)
{

}

CPolinomios::~CPolinomios(){
}

Sistema * CPolinomios::invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                               Var * k, Var* ret, QString exp_nume __attribute__((unused)), QString exp_deno __attribute__((unused))){
    return new CPolinomios(nombre, numerador, denominador, k, ret);
}

Sistema::tipo_planta CPolinomios::getClass(){
    return cof_polinomios;
}

QString CPolinomios::getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                              qreal k, qreal ret, qreal omega){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;


    es +=  "(" +  QString::number(k) + "*(";



    for (qint32 i = 1; i < sizeNum; i++){


        es += "(" + QString::number(numerador->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeNum - i)+ ") +";

    }


    if (sizeNum > 0){
        es += "(" + QString::number(numerador->last()) + ")) / (";
    } else {
        es += "(1))/(";
    }


    for (qint32 i = 1; i < sizeDen; i++){


        es += "(" + QString::number(denominador->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeDen - i) + ") +";

    }

    if (sizeDen > 0){
        es += "(" + QString::number(denominador->last()) + ")))";
    }else {
        es += "(1)))";
    }

    if (ret != 0){

        es += "* e^(i*" + QString::number(omega) + "*" + QString::number(ret) +")";
    }


    return es;
}

QString CPolinomios::getExpr(qreal w){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isVariable()){
        es += "(kv*(";
    }else {
        es += "(" + QString::number(k->getNominal()) + "*(";
    }


    for (qint32 i = 1; i < sizeNum; i++){

        if (numerador->at(i-1)->isVariable()){
            es += "(" + numerador->at(i-1)->getNombre() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            es += "(" + QString::number(numerador->at(i-1)->getNominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (numerador->size() > 0){
        if (numerador->last()->isVariable()){
            es += "(" + numerador->last()->getNombre() + ")) / (";
        }else{
            es += "(" + QString::number(numerador->last()->getNominal()) + ")) / (";
        }
    } else {
        es += "(1)) / (";
    }

    for (qint32 i = 1; i < sizeDen; i++){

        if (denominador->at(i-1)->isVariable()){
            es += "(" + denominador->at(i-1)->getNombre() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            es += "(" + QString::number(denominador->at(i-1)->getNominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }


    if (denominador->size() > 0){
        if (denominador->last()->isVariable()){
            es += "(" + denominador->last()->getNombre() + ")))";
        }else{
            es += "(" + QString::number(denominador->last()->getNominal()) + ")))";
        }
    } else {
        es += "(1)))";
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

QString CPolinomios::getExpr(){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isVariable()){
        es += "(kv*(";
    }else {
        es +="(" + QString::number(k->getNominal()) + "*(";
    }


    for (qint32 i = 1; i < sizeNum; i++){

        if (numerador->at(i-1)->isVariable()){
            es += "(" + numerador->at(i-1)->getNombre() + "*s^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            es += "(" + QString::number(numerador->at(i-1)->getNominal()) + "*s^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (numerador->size() > 0){
        if (numerador->last()->isVariable()){
            es += "(" + numerador->last()->getNombre() + ")) / (";
        }else{
            es += "(" + QString::number(numerador->last()->getNominal()) + ")) / (";
        }
    } else {
        es += "(1)) / (";
    }

    for (qint32 i = 1; i < sizeDen; i++){

        if (denominador->at(i-1)->isVariable()){
            es += "(" + denominador->at(i-1)->getNombre() + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            es += "(" + QString::number(denominador->at(i-1)->getNominal()) + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }

    if (denominador->size() > 0){
        if (denominador->last()->isVariable()){
            es += "(" + denominador->last()->getNombre() + ")))";
        }else{
            es += "(" + QString::number(denominador->last()->getNominal()) + ")))";
        }
    } else {
        es += "(1)))";
    }

    if (ret->getNominal() != 0){
        if (ret->isVariable()){
            es += "e^(s*ret)";
        }else{
            es += " e^(s*" + QString::number(ret->getNominal()) +")";
        }
    }

    return es;
}

std::complex <qreal> CPolinomios::getPuntoNume(QVector <qreal> * nume, qreal omega){

    if (nume->size() == 0){
        return std::complex <qreal> (1, 0);
    }

    qint32 sizeNum = nume->size();
    QString es = "(";


    for (qint32 i = 1; i < sizeNum; i++){
        es += "(" + QString::number(nume->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeNum - i)+ ") +";
    }

    es += "(" + QString::number(nume->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> CPolinomios::getPuntoDeno(QVector <qreal> * deno, qreal omega){

    if (deno->size() == 0){
        return std::complex <qreal> (1, 0);
    }

    qint32 sizeDen = deno->size();
    QString es = "(";


    for (qint32 i = 1; i < sizeDen; i++){
        es += "(" + QString::number(deno->at(i-1)) + "*(" + QString::number(omega) + "*i)^" +
                QString::number(sizeDen - i)+ ") +";
    }

    es += "(" + QString::number(deno->last()) + "))";

    mup::ParserX p (mup::pckALL_COMPLEX);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}
