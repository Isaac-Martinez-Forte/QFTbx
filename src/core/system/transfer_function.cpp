#include "transfer_function.h"

using namespace std;
using namespace mup;

namespace qftbx {

TransferFunction::TransferFunction(QString name, QVector <Parameter*> * numerator, QVector <Parameter*> * denominator,
        Parameter * k, Parameter * delay) :
LtiSystem(name) {
    m_numerator = numerator;
    m_denominator = denominator;

    m_gain = k;
    m_delay = delay;
}

TransferFunction::~TransferFunction() {

    //The system owns its Parameters and vectors (whoever builds one hands
    //over ownership; the GUI passes copies). releaseOwnership() disarms
    //deletion for structures that share the pointers.
    if (m_ownsData) {
        qDeleteAll(*m_numerator);
        delete m_numerator;
        qDeleteAll(*m_denominator);
        delete m_denominator;
        delete m_gain;
        delete m_delay;
    }
}

void TransferFunction::releaseOwnership() {
    m_ownsData = false;
}

QVector <Parameter*> * TransferFunction::numerator() {
    return m_numerator;
}

QVector <Parameter*> * TransferFunction::denominator() {
    return m_denominator;
}

Parameter * TransferFunction::gain() {
    return m_gain;
}

Parameter * TransferFunction::delay() {
    return m_delay;
}

std::complex <qreal> TransferFunction::evaluate(QVector <qreal> * numerator, QVector <qreal> * denominator,
        qreal k, qreal delay, qreal omega) {
    ParserX p(pckALL_COMPLEX);

    p.SetExpr(expression(numerator, denominator, k, delay, omega).toStdString());

    return p.Eval().GetComplex();
}

std::complex <qreal> TransferFunction::evaluate(qreal w) {

    ParserX p(pckALL_COMPLEX);

    p.EnableAutoCreateVar(true);

    QString expr;

    foreach(Parameter * n, *m_numerator) {

        if (n->isUncertain()) {
            expr = n->name() + "=" + QString::number(n->nominal());
            p.SetExpr(expr.toStdString());
            p.Eval();
        }
    }

    foreach(Parameter * d, *m_denominator) {
        if (d->isUncertain()) {
            expr = d->name() + "=" + QString::number(d->nominal());
            p.SetExpr(expr.toStdString());
            p.Eval();
        }
    }

    if (m_gain->isUncertain()) {
        expr = m_gain->name() + "=" + QString::number(m_gain->nominal());
        p.SetExpr(expr.toStdString());
        p.Eval();
    }

    if (m_delay->isUncertain()) {
        expr = m_delay->name() + "=" + QString::number(m_delay->nominal());
        p.SetExpr(expr.toStdString());
        p.Eval();
    }

    expr = expression(w);

    p.SetExpr(expr.toStdString());

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

    Parameter * k = m_gain->clone();

    Parameter * delay = m_delay->clone();

    foreach(Parameter * v, *m_numerator) {
        n->append(v->clone());
    }

    foreach(Parameter * v, *m_denominator) {
        d->append(v->clone());
    }


    return this->create(this->name(), n, d, k, delay);
}

} // namespace qftbx
