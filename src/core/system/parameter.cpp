#include "parameter.h"

using namespace mup;
using namespace std;

Parameter::Parameter(QString nombre, QPointF rango, qreal nominal, QString exp)
{
    this->nombre = nombre;

    if (rango.x() > rango.y()){
        this->rango.setX(rango.y());
        this->rango.setY(rango.x());
    }else {
        this->rango = rango;
    }

    m_nominal = nominal;
    variable = true;

    if (exp == nullptr || exp.isEmpty()) {
        //Sin reparametrizacion la expresion es la propia variable, igual
        //que en el constructor de tres argumentos.
        this->exp = nombre;
        e = false;
    } else {
        this->exp = exp;
        e = true;
    }
}

Parameter::Parameter(QString nombre, QPointF rango, qreal nominal){
    this->nombre = nombre;

    if (rango.x() > rango.y()){
        this->rango.setX(rango.y());
        this->rango.setY(rango.x());
    }else {
        this->rango = rango;
    }

    m_nominal = nominal;
    this->exp = nombre;

    variable = true;


    e = false;
}

Parameter::Parameter (QPointF rango){
    if (rango.x() > rango.y()){
        this->rango.setX(rango.y());
        this->rango.setY(rango.x());
    }else {
        this->rango = rango;
    }

    m_nominal = 0;
    variable = false;
    e = false;
}

Parameter::Parameter(const Parameter &obj){
    this->nombre = obj.nombre;
    this->rango = obj.rango;
    m_nominal = obj.m_nominal;
    this->variable = obj.variable;
    this->exp = obj.exp;
    this->e = obj.e;
}

Parameter::Parameter() {
   m_nominal = 0;
   variable = false;
   e = false;
}

Parameter::Parameter (qreal valor){
    m_nominal = valor;
    nombre = QString::number(m_nominal);
    variable = false;
    this->rango= QPointF (m_nominal, m_nominal);
    this->exp = nombre;
}

Parameter::Parameter (QString nombre, qreal valor){
    m_nominal = valor;
    this->nombre = nombre;
    variable = false;
    this->rango= QPointF (m_nominal, m_nominal);
    this->exp = nombre;
}


bool Parameter::isUncertain(){
    return variable;
}

void Parameter::setUncertain(bool a) {
    variable = a;
}

QString Parameter::name(){
    return nombre;
}

QPointF Parameter::range(){

    if (!variable){
        return rango;
    }

    if (!e){
        return rango;
    }

    QPointF punto;

    //Los Value deben declararse antes que el parser: este guarda punteros a
    //ellos (Variable(&v)) y se destruyen en orden inverso a su declaracion.
    Value v(rango.x());
    Value v2(rango.y());

    mup::ParserX p;

    p.SetExpr(exp.toStdString());
    p.DefineVar(nombre.toStdString(), Variable(&v));

    punto.setX(p.Eval().GetFloat());

    p.RemoveVar(nombre.toStdString());
    p.DefineVar(nombre.toStdString(), Variable(&v2));

    punto.setY(p.Eval().GetFloat());

    return punto;
}

qreal Parameter::nominal(){

    if (!variable){
        return m_nominal;
    }

    if (!e){
        return m_nominal;
    }

    //Value antes que el parser: ver comentario en range().
    Value v(m_nominal);

    mup::ParserX p;

    p.SetExpr(exp.toStdString());
    p.DefineVar(nombre.toStdString(), Variable(&v));

    return p.Eval().GetFloat();
}

void Parameter::setName(QString nombre){
    this->nombre = nombre;
}

void Parameter::setRange(QPointF rango){
    this->rango = rango;
}

void Parameter::setNominal(qreal nominal){
    m_nominal = nominal;
}

QString Parameter::expression(){
    return exp;
}

QPointF Parameter::rawRange(){
    return rango;
}

qreal Parameter::rawNominal(){
    return m_nominal;
}

Parameter * Parameter::clone(){

    if (!variable){
        //Se conserva el nombre: una constante con nombre ("kv") no debe
        //convertirse en una constante llamada por su valor.
        return new Parameter (this->nombre, m_nominal);
    }

    if (!e){
        return new Parameter (this->nombre, this->rango, m_nominal);
    }

    return new Parameter (this->nombre, this->rango, m_nominal, this->exp);
}

QVector <Parameter*> * Parameter::cloneVector(QVector <Parameter*> * origen){

    QVector <Parameter*> * copia = new QVector <Parameter*> ();
    copia->reserve(origen->size());

    foreach (Parameter * var, *origen) {
        copia->append(var->clone());
    }

    return copia;
}
