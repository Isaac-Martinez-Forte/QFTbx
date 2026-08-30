#include "transfer_function.h"

using namespace std;
using namespace mup;

TransferFunction::TransferFunction(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
        Parameter * k, Parameter * ret) :
LtiSystem(nombre) {
    this->numerador = numerador;
    this->denominador = denominador;

    this->k = k;
    this->ret = ret;
}

TransferFunction::~TransferFunction() {

    //La planta es duena de sus Parameter y de los vectores (quien construye una
    //planta le cede la propiedad; la GUI entrega copias). releaseOwnership() anula
    //la propiedad para el caso de estructuras que comparten los punteros.
    if (b) {
        qDeleteAll(*numerador);
        delete numerador;
        qDeleteAll(*denominador);
        delete denominador;
        delete k;
        delete ret;
    }
}

void TransferFunction::releaseOwnership() {
    b = false;
}

QVector <Parameter*> * TransferFunction::numerator() {
    return numerador;
}

QVector <Parameter*> * TransferFunction::denominator() {
    return denominador;
}

Parameter * TransferFunction::gain() {
    return k;
}

Parameter * TransferFunction::delay() {
    return ret;
}

std::complex <qreal> TransferFunction::evaluate(QVector <qreal> * numerador, QVector <qreal> * denominador,
        qreal k, qreal ret, qreal omega) {
    ParserX p(pckALL_COMPLEX);

    p.SetExpr(expression(numerador, denominador, k, ret, omega).toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> TransferFunction::evaluate(qreal w) {

    ParserX p(pckALL_COMPLEX);

    p.EnableAutoCreateVar(true);

    QString es;

    foreach(Parameter * n, *numerador) {

        if (n->isUncertain()) {
            es = n->name() + "=" + QString::number(n->nominal());
            p.SetExpr(es.toStdString());
            p.Eval();
        }
    }

    foreach(Parameter * d, *denominador) {
        if (d->isUncertain()) {
            es = d->name() + "=" + QString::number(d->nominal());
            p.SetExpr(es.toStdString());
            p.Eval();
        }
    }

    if (k->isUncertain()) {
        es = k->name() + "=" + QString::number(k->nominal());
        p.SetExpr(es.toStdString());
        p.Eval();
    }

    if (ret->isUncertain()) {
        es = ret->name() + "=" + QString::number(ret->nominal());
        p.SetExpr(es.toStdString());
        p.Eval();
    }

    es = expression(w);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}

QVector <std::complex <qreal> > * TransferFunction::evaluate(QVector <qreal> * omega) {

    QVector <std::complex <qreal> > * resultado = new QVector <std::complex <qreal> > ();

    foreach(qreal o, *omega) {
        resultado->append(evaluate(o));
    }

    return resultado;
}

QString TransferFunction::numeratorString() {
    return QString();
}

QString TransferFunction::denominatorString() {
    return QString();
}

LtiSystem * TransferFunction::clone() {

    QVector <Parameter *> * n = new QVector <Parameter *> ();
    QVector <Parameter *> * d = new QVector <Parameter *> ();

    Parameter * k = this->k->clone();

    Parameter * ret = this->ret->clone();

    foreach(Parameter * v, *numerador) {
        n->append(v->clone());
    }

    foreach(Parameter * v, *denominador) {
        d->append(v->clone());
    }


    return this->create(this->name(), n, d, k, ret);
}
