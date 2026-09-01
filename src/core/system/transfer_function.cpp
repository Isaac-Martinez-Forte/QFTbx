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

namespace {

//The nominal of every parameter, in order. Uncertain ones contribute their
//nominal here exactly as they did through the parser, which bound each name
//to its nominal before evaluating.
std::vector<qreal> nominalsOf(std::vector<Parameter> & parameters)
{
    std::vector<qreal> values;
    values.reserve(parameters.size());

    for (Parameter & parameter : parameters) {
        values.push_back(parameter.nominal());
    }

    return values;
}

} // namespace

std::complex <qreal> TransferFunction::evaluate(qreal w) {
    //Direct complex arithmetic: see valueAt() in the header for what this
    //replaced and why.
    return valueAt(w, nominalsOf(m_numerator), nominalsOf(m_denominator),
                   m_gain.nominal(), m_delay.nominal());
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
