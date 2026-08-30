#include "var.h"

using namespace mup;
using namespace std;

Var::Var(QString nombre, QPointF rango, qreal nominal, QString exp)
{
    this->nombre = nombre;

    if (rango.x() > rango.y()){
        this->rango.setX(rango.y());
        this->rango.setY(rango.x());
    }else {
        this->rango = rango;
    }

    this->nominal = nominal;
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

Var::Var(QString nombre, QPointF rango, qreal nominal){
    this->nombre = nombre;

    if (rango.x() > rango.y()){
        this->rango.setX(rango.y());
        this->rango.setY(rango.x());
    }else {
        this->rango = rango;
    }

    this->nominal = nominal;
    this->exp = nombre;

    variable = true;


    e = false;
}

Var::Var (QPointF rango){
    if (rango.x() > rango.y()){
        this->rango.setX(rango.y());
        this->rango.setY(rango.x());
    }else {
        this->rango = rango;
    }

    nominal = 0;
    variable = false;
    e = false;
}

Var::Var(const Var &obj){
    this->nombre = obj.nombre;
    this->rango = obj.rango;
    this->nominal = obj.nominal;
    this->variable = obj.variable;
    this->exp = obj.exp;
    this->e = obj.e;
}

Var::Var() {
   nominal = 0;
   variable = false;
   e = false;
}

Var::Var (qreal valor){
    nominal = valor;
    nombre = QString::number(nominal);
    variable = false;
    this->rango= QPointF (nominal, nominal);
    this->exp = nombre;
}

Var::Var (QString nombre, qreal valor){
    nominal = valor;
    this->nombre = nombre;
    variable = false;
    this->rango= QPointF (nominal, nominal);
    this->exp = nombre;
}


bool Var::isVariable(){
    return variable;
}

void Var::setVariable(bool a) {
    variable = a;
}

QString Var::getNombre(){
    return nombre;
}

QPointF Var::getRango(){

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

qreal Var::getNominal(){

    if (!variable){
        return nominal;
    }

    if (!e){
        return nominal;
    }

    //Value antes que el parser: ver comentario en getRango().
    Value v(nominal);

    mup::ParserX p;

    p.SetExpr(exp.toStdString());
    p.DefineVar(nombre.toStdString(), Variable(&v));

    return p.Eval().GetFloat();
}

void Var::setNombre(QString nombre){
    this->nombre = nombre;
}

void Var::setRango(QPointF rango){
    this->rango = rango;
}

void Var::setNominal(qreal nominal){
    this->nominal = nominal;
}

QString Var::getExp(){
    return exp;
}

QPointF Var::getR(){
    return rango;
}

qreal Var::getN(){
    return nominal;
}

Var * Var::clone(){

    if (!variable){
        //Se conserva el nombre: una constante con nombre ("kv") no debe
        //convertirse en una constante llamada por su valor.
        return new Var (this->nombre, this->nominal);
    }

    if (!e){
        return new Var (this->nombre, this->rango, this->nominal);
    }

    return new Var (this->nombre, this->rango, this->nominal, this->exp);
}

QVector <Var*> * Var::clonarVector(QVector <Var*> * origen){

    QVector <Var*> * copia = new QVector <Var*> ();
    copia->reserve(origen->size());

    foreach (Var * var, *origen) {
        copia->append(var->clone());
    }

    return copia;
}
