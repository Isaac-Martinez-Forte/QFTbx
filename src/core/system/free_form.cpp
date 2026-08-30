#include "free_form.h"

using namespace std;
using namespace mup;

namespace qftbx {

FreeForm::FreeForm(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador, Parameter * k,
                           Parameter* ret, QString exp_nume, QString exp_deno)
    :TransferFunction (nombre, numerador, denominador, k, ret)
{
    this->exp_nume = exp_nume;
    this->exp_deno = exp_deno;
}

std::complex <qreal> FreeForm::evaluate (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                             qreal k, qreal ret, qreal omega){ //TODO ver que hacer con esto
    return complex <qreal> ();
}

QString FreeForm::expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                               qreal k, qreal ret, qreal omega){//TODO ver que hacer con esto
    return "";
}

std::complex <qreal> FreeForm::evaluateNumerator(QVector <qreal> * nume, qreal omega){


    return complex <qreal> ();
}

std::complex <qreal> FreeForm::evaluateDenominator(QVector <qreal> * deno, qreal omega){


    return complex <qreal> ();
}

QString FreeForm::expression(qreal w){

    QString n = exp_nume;
    QString d = exp_deno;

    QString es = k->expression() + "*(" + n.replace("s", "(" + QString::number(w) + "*i)") + ")/(" +
            d.replace("s", "(" + QString::number(w) + "*i)") + ")";


    //El retardo puro es e^(-s*tau) => e^(-i*w*tau). Se emite si es variable
    //(aunque su nominal sea 0, para que el barrido de templates lo recorra)
    //o si es una constante no nula.
    if (ret->isUncertain()){
        es += "* e^(-i*" + QString::number(w) + "*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += "* e^(-i*" + QString::number(w) + "*" +
                QString::number(ret->nominal()) +")";
    }

    return es;
}

QString FreeForm::expression(){
    QString es = k->expression() + "*(" + exp_nume + ")/(" + exp_deno + ")";

    if (ret->isUncertain()){
        es += " * e^(-s*" + ret->name() + ")";
    }else if (ret->nominal() != 0){
        es += " * e^(-s*" + QString::number(ret->nominal()) +")";
    }

    return es;
}

LtiSystem::SystemType FreeForm::type(){
    return SystemType::FreeForm;
}

LtiSystem * FreeForm::create(QString nombre, QVector<Parameter *> *numerador, QVector<Parameter *> *denominador,
                               Parameter *k, Parameter *ret, QString exp_nume, QString exp_deno){

    //Un retardo no especificado equivale a retardo cero.
    return new FreeForm (nombre, numerador, denominador, k,
                             ret == NULL ? new Parameter(0.0) : ret, exp_nume, exp_deno);
}


QString FreeForm::numeratorString(){
    return exp_nume;
}

QString FreeForm::denominatorString(){
    return exp_deno;
}

LtiSystem * FreeForm::clone(){

    Parameter * k = this->k->clone();
    Parameter * ret = this->ret->clone();

    return this->create(this->name(), Parameter::cloneVector(numerador),
                        Parameter::cloneVector(denominador), k, ret,
                        this->exp_nume, this->exp_deno);
}

} // namespace qftbx
