#include "polynomial_form.h"

using namespace std;

namespace qftbx {

PolynomialForm::PolynomialForm(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador, Parameter * k, Parameter* ret):
    TransferFunction(nombre, numerador, denominador, k , ret)
{

}

PolynomialForm::~PolynomialForm(){
}

LtiSystem * PolynomialForm::create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                               Parameter * k, Parameter* ret, QString exp_nume __attribute__((unused)), QString exp_deno __attribute__((unused))){
    //Un retardo no especificado equivale a retardo cero.
    return new PolynomialForm(nombre, numerador, denominador, k, ret == NULL ? new Parameter(0.0) : ret);
}

LtiSystem::SystemType PolynomialForm::type(){
    return SystemType::PolynomialForm;
}

QString PolynomialForm::expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
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

        es += "* e^(-i*" + QString::number(omega) + "*" + QString::number(ret) +")";
    }


    return es;
}

QString PolynomialForm::expression(qreal w){

    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isUncertain()){
        es += "(" + k->name() + "*(";
    }else {
        es += "(" + QString::number(k->nominal()) + "*(";
    }


    for (qint32 i = 1; i < sizeNum; i++){

        if (numerador->at(i-1)->isUncertain()){
            es += "(" + numerador->at(i-1)->name() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            es += "(" + QString::number(numerador->at(i-1)->nominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (numerador->size() > 0){
        if (numerador->last()->isUncertain()){
            es += "(" + numerador->last()->name() + ")) / (";
        }else{
            es += "(" + QString::number(numerador->last()->nominal()) + ")) / (";
        }
    } else {
        es += "(1)) / (";
    }

    for (qint32 i = 1; i < sizeDen; i++){

        if (denominador->at(i-1)->isUncertain()){
            es += "(" + denominador->at(i-1)->name() + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            es += "(" + QString::number(denominador->at(i-1)->nominal()) + "*(" + QString::number(w) + "*i)^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }


    if (denominador->size() > 0){
        if (denominador->last()->isUncertain()){
            es += "(" + denominador->last()->name() + ")))";
        }else{
            es += "(" + QString::number(denominador->last()->nominal()) + ")))";
        }
    } else {
        es += "(1)))";
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

QString PolynomialForm::expression(){
    qint32 sizeDen = denominador->size();
    qint32 sizeNum = numerador->size();

    QString es;

    if (k->isUncertain()){
        es += "(" + k->name() + "*(";
    }else {
        es +="(" + QString::number(k->nominal()) + "*(";
    }


    for (qint32 i = 1; i < sizeNum; i++){

        if (numerador->at(i-1)->isUncertain()){
            es += "(" + numerador->at(i-1)->name() + "*s^" +
                    QString::number(sizeNum - i) + ") +";
        } else {
            es += "(" + QString::number(numerador->at(i-1)->nominal()) + "*s^" +
                    QString::number(sizeNum - i)+ ") +";
        }
    }

    if (numerador->size() > 0){
        if (numerador->last()->isUncertain()){
            es += "(" + numerador->last()->name() + ")) / (";
        }else{
            es += "(" + QString::number(numerador->last()->nominal()) + ")) / (";
        }
    } else {
        es += "(1)) / (";
    }

    for (qint32 i = 1; i < sizeDen; i++){

        if (denominador->at(i-1)->isUncertain()){
            es += "(" + denominador->at(i-1)->name() + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        } else {
            es += "(" + QString::number(denominador->at(i-1)->nominal()) + "*s^" +
                    QString::number(sizeDen - i) + ") +";
        }
    }

    if (denominador->size() > 0){
        if (denominador->last()->isUncertain()){
            es += "(" + denominador->last()->name() + ")))";
        }else{
            es += "(" + QString::number(denominador->last()->nominal()) + ")))";
        }
    } else {
        es += "(1)))";
    }

    if (ret->isUncertain()){
        es += " * e^(-s*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += " * e^(-s*" + QString::number(ret->nominal()) +")";
    }

    return es;
}

std::complex <qreal> PolynomialForm::evaluateNumerator(QVector <qreal> * nume, qreal omega){

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

std::complex <qreal> PolynomialForm::evaluateDenominator(QVector <qreal> * deno, qreal omega){

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

} // namespace qftbx
