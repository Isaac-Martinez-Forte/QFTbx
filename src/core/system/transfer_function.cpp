#include "transfer_function.h"

using namespace std;
using namespace mup;

namespace qftbx {

TransferFunction::TransferFunction(QString name, std::vector <Parameter> numerator,
        std::vector <Parameter> denominator, Parameter k, Parameter delay) :
LtiSystem(name),
m_gain(std::move(k)),
m_delay(std::move(delay)),
m_numerator(std::move(numerator)),
m_denominator(std::move(denominator)) {
}

std::vector <Parameter> & TransferFunction::numerator() {
    return m_numerator;
}

std::vector <Parameter> & TransferFunction::denominator() {
    return m_denominator;
}

Parameter & TransferFunction::gain() {
    return m_gain;
}

Parameter & TransferFunction::delay() {
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

    const auto bindNominal = [&](Parameter & parameter) {
        if (parameter.isUncertain()) {
            expr = parameter.name() + "=" + QString::number(parameter.nominal());
            p.SetExpr(expr.toStdString());
            p.Eval();
        }
    };

    for (Parameter & n : m_numerator) {
        bindNominal(n);
    }

    for (Parameter & d : m_denominator) {
        bindNominal(d);
    }

    bindNominal(m_gain);
    bindNominal(m_delay);

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

    //Values copy themselves: no per-parameter cloning any more. FreeForm
    //overrides this to carry its expression strings too.
    return this->create(this->name(), m_numerator, m_denominator, m_gain, m_delay);
}

} // namespace qftbx
