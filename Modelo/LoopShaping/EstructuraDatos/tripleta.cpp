#include "tripleta.h"

using namespace tools;

Tripleta::Tripleta(qreal index, Sistema * sistema, flags_box flags){
    this->index = index;
    this->sistema = sistema;
    this->flags = flags;
}

Tripleta::Tripleta(qreal index, Sistema * sistema,  QVector <struct datos_caja> * datos) : Tripleta(index, sistema) {
    this->datos = datos;
}

Tripleta::~Tripleta() {

    if (!bo) {
        if (sistema != nullptr){
            delete sistema;
        }
    }

    if (datos != nullptr){
        datos->clear();
    }

    if (puntosCorte != nullptr){
        puntosCorte->clear();
    }

    if (feasible != nullptr){
        feasible->clear();
    }
}

Tripleta & Tripleta::operator=(const Tripleta &c) {

    if(this != &c) {
        index = c.index;
        flags = c.flags;
        sistema = c.sistema;
    }

    return *this;
}

bool Tripleta::operator==(const Tripleta &c) const {
    return index == c.index;
}
bool Tripleta::operator!=(const Tripleta &c) const {
    return index != c.index;
}
bool Tripleta::operator<(const Tripleta &c) const {
    return index < c.index;
}
bool Tripleta::operator>(const Tripleta &c) const {
    return index > c.index;
}
bool Tripleta::operator<=(const Tripleta &c) const {
    return index <= c.index;
}
bool Tripleta::operator>=(const Tripleta &c) const {
    return index >= c.index;
}

flags_box Tripleta::getFlags() const
{
    return flags;
}

void Tripleta::setFlags(const flags_box &value)
{
    flags = value;
}

QVector<datos_caja> * Tripleta::getDatos() const
{
    return datos;
}

void Tripleta::setDatos(QVector<datos_caja> *value)
{
    datos = value;
}

Sistema * Tripleta::getSistema() const
{
    return sistema;
}

void Tripleta::setSistema(Sistema *value)
{
    sistema = value;
}

void Tripleta::setFeasible(QVector<bool> *value)
{
    feasible = value;
}

QVector<qreal> * Tripleta::getPuntosCorte() const
{
    return puntosCorte;
}

void Tripleta::setPuntosCorte(QVector<qreal> *value)
{
    puntosCorte = value;
}
void Tripleta::borrar() {
    bo = true;
}
