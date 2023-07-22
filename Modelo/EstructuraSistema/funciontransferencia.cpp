#include "funciontransferencia.h"

using namespace std;
using namespace mup;

FuncionTransferencia::FuncionTransferencia(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
        Var * k, Var * ret) :
Sistema(nombre) {
    this->numerador = numerador;
    this->denominador = denominador;

    this->k = k;
    this->ret = ret;
}

FuncionTransferencia::FuncionTransferencia(FuncionTransferencia * datos) :
FuncionTransferencia(datos->getNombre(), datos->numerador, datos->denominador, datos->k, datos->ret) {
}

FuncionTransferencia::~FuncionTransferencia() {

    if (b) {
        numerador->clear();
        denominador->clear();
        delete k;
        delete ret;
    }
}

void FuncionTransferencia::noBorrar() {
    b = false;
}

QVector <Var*> * FuncionTransferencia::getNumerador() {
    return numerador;
}

QVector <Var*> * FuncionTransferencia::getDenominador() {
    return denominador;
}

Var * FuncionTransferencia::getK() {
    return k;
}

Var * FuncionTransferencia::getRet() {
    return ret;
}

std::complex <qreal> FuncionTransferencia::getPunto(QVector <qreal> * numerador, QVector <qreal> * denominador,
        qreal k, qreal ret, qreal omega) {
    ParserX p(pckALL_COMPLEX);

    p.SetExpr(getExpr(numerador, denominador, k, ret, omega).toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> FuncionTransferencia::getPunto(qreal w) {

    ParserX p(pckALL_COMPLEX);

    p.EnableAutoCreateVar(true);

    QString es;

    foreach(Var * n, *numerador) {

        if (n->isVariable()) {
            es = n->getNombre() + "=" + QString::number(n->getNominal());
            p.SetExpr(es.toStdString());
            p.Eval();
        }
    }

    foreach(Var * d, *denominador) {
        if (d->isVariable()) {
            es = d->getNombre() + "=" + QString::number(d->getNominal());
            p.SetExpr(es.toStdString());
            p.Eval();
        }
    }

    if (k->isVariable()) {
        es = k->getNombre() + "=" + QString::number(k->getNominal());
        p.SetExpr(es.toStdString());
        p.Eval();
    }

    if (ret->isVariable()) {
        es = ret->getNombre() + "=" + QString::number(ret->getNominal());
        p.SetExpr(es.toStdString());
        p.Eval();
    }

    es = getExpr(w);

    p.SetExpr(es.toStdString());

    return p.Eval().GetComplex();
}

QVector <std::complex <qreal> > * FuncionTransferencia::getPunto(QVector <qreal> * omega) {

    QVector <std::complex <qreal> > * resultado = new QVector <std::complex <qreal> > ();

    foreach(qreal o, *omega) {
        resultado->append(getPunto(o));
    }

    return resultado;
}

QString FuncionTransferencia::getNumeradorString() {
    return QString();
}

QString FuncionTransferencia::getDenominadorString() {
    return QString();
}

Sistema * FuncionTransferencia::clone() {

    QVector <Var *> * n = new QVector <Var *> ();
    QVector <Var *> * d = new QVector <Var *> ();

    Var * k = this->k->clone();

    Var * ret = this->ret->clone();

    foreach(Var * v, *numerador) {
        n->append(v->clone());
    }

    foreach(Var * v, *denominador) {
        d->append(v->clone());
    }


    return this->invoke(this->getNombre(), n, d, k, ret);
}
