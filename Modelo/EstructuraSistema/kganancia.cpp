#include "kganancia.h"

using namespace std;
using namespace mup;

KGanancia::KGanancia(QString nombre, QVector<Var *> *numerador, QVector<Var *> *denominador, Var *k, Var *ret):
    PolosCeros(nombre, numerador, denominador,k,ret)
{
}

KGanancia::~KGanancia(){
}

std::shared_ptr<Sistema> KGanancia::invoke(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                             Var * k, Var* ret, QString exp_nume __attribute__((unused)), QString exp_deno __attribute__((unused))){
    return std::make_shared<KGanancia>(nombre, numerador, denominador, k, ret = NULL ? new Var(0) : ret);
}



QString KGanancia::getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
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
        es += "* e^(i*" + QString::number(omega) + "*" + QString::number(ret) +")";
    }


    return es;
}

QString KGanancia::getExpr(qreal w){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isVariable()){
        es += k->getNombre() + "*(";
    }else {
        es += QString::number(k->getNominal()) + "*(";
    }

    if (numerador->isEmpty()){
        es += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            if (numerador->at(i)->isVariable()){
                es += "((" + QString::number(w) + "*i) + " + numerador->at(i)->getNombre() + ") *";
            } else {
                es += "(("+ QString::number(w) + "*i) +" + QString::number(numerador->at(i)->getNominal()) + ") *";
            }
        }

        if(numerador->last()->isVariable()){
            es += "((" + QString::number(w) + "*i) + " + numerador->last()->getNombre() + ")) / (";
        } else {
            es += "((" + QString::number(w) + "*i) + " + QString::number(numerador->last()->getNominal()) + ")) / (";
        }
    }


    if (denominador->isEmpty()){
        es += "1)";
    } else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            if (denominador->at(i)->isVariable()){
                es += "((" + QString::number(w) + "*i) + " + denominador->at(i)->getNombre() + ") *";
            } else {
                es += "(("+ QString::number(w) + "*i) + " + QString::number(denominador->at(i)->getNominal()) + ") *";
            }
        }


        if (denominador->last()->isVariable()){
            es += "((" + QString::number(w) + "*i) + " + denominador->last()->getNombre() + "))";
        }else{
            es += "(("+ QString::number(w) + "*i) + " + QString::number(denominador->last()->getNominal()) + "))";
        }
    }


    if (ret->getNominal() != 0){

        if (ret->isVariable()){
            es += "* e^(i*" + QString::number(w) + "*" + ret->getNombre() + ")";
        }else{
            es += "* e^(i*" + QString::number(w) + "*" + QString::number(ret->getNominal()) +")";
        }
    }

    return es;
}


Sistema::tipo_planta KGanancia::getClass(){
    return k_ganancia;
}


QString KGanancia::getExpr(){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isVariable()){
        es += k->getNombre() + "*(";
    }else {
        es += QString::number(k->getNominal()) + "*(";
    }

    if (numerador->isEmpty()){
        es += "1) / (";
    } else {
        for (qint32 i = 0; i < sizeNum-1; i++){

            if (numerador->at(i)->isVariable()){
                es += "(s + " + numerador->at(i)->getNombre() + ") *";
            } else {
                es += "(s +" + QString::number(numerador->at(i)->getNominal()) + ") *";
            }
        }

        if(numerador->last()->isVariable()){
            es += "(s + " + numerador->last()->getNombre() + ")) / (";
        } else {
            es += "(s + " + QString::number(numerador->last()->getNominal()) + ")) / (";
        }
    }


    if (denominador->isEmpty()){
        es += "1)";
    }else {
        for (qint32 i = 0; i < sizeDen-1; i++){

            if (denominador->at(i)->isVariable()){
                es += "(s + " + denominador->at(i)->getNombre() + ") *";
            } else {
                es += "(s + " + QString::number(denominador->at(i)->getNominal()) + ") *";
            }
        }

        if (denominador->last()->isVariable()){
            es += "(s + " + denominador->last()->getNombre() + ")) *";
        }else{
            es += "(s + " + QString::number(denominador->last()->getNominal()) + ")) *";
        }
    }

    if (ret->isVariable()){
        es += "e^(s*" + ret->getNombre() + ")";
    }else{
        es += " e^(s*" + QString::number(ret->getNominal()) +")";
    }

    return es;
}


std::complex <qreal> KGanancia::getPuntoNume(QVector <qreal> * nume, qreal omega){

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

std::complex <qreal> KGanancia::getPuntoDeno(QVector <qreal> * deno, qreal omega){

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
