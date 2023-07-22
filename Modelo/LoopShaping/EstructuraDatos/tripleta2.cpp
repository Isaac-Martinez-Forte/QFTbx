#include "tripleta2.h"


using namespace cxsc;
using namespace tools;

Tripleta2::Tripleta2(qreal index, Sistema * sistema, flags_box flags) : Tripleta (index, sistema, flags) {
    recorteActivado = true;
}

Tripleta2::Tripleta2(qreal index, Sistema * sistema,  QVector <data_box *> * datos) : Tripleta(index, sistema, datos) {
    recorteActivado = true;
}

Tripleta2::~Tripleta2() {

    if (b2) {
        if (terNume != nullptr) {
            terNume->clear();
        }

        if (terDeno != nullptr) {
            terDeno->clear();
        }

        if (frecuenciasFeasible != nullptr) {
            frecuenciasFeasible->clear();
        }
    }

    if (numeradorInfMag != nullptr) {
        numeradorInfMag->clear();
    }

    if (numeradorSupMag != nullptr) {
        numeradorSupMag->clear();
    }

    if (denominadorInfMag != nullptr) {
        denominadorInfMag->clear();
    }

    if (denominadorSupMag != nullptr) {
        denominadorSupMag->clear();
    }

    if (numeradorInfFas != nullptr) {
        numeradorInfFas->clear();
    }

    if (numeradorSupFas != nullptr) {
        numeradorSupFas->clear();
    }

    if (denominadorInfFas != nullptr) {
        denominadorInfFas->clear();
    }

    if (denominadorSupFas != nullptr) {
        denominadorSupFas->clear();
    }

    if (kInf != nullptr) {
        delete kInf;
    }

    if (kSup != nullptr) {
        delete kSup;
    }

    if (datosCortesBoundaries != nullptr) {
        datosCortesBoundaries->clear();
    }
}

void Tripleta2::setRecorteActivado (bool recorteActivado) {
    this->recorteActivado = recorteActivado;
}

bool Tripleta2::isRecorteActivado (){
    return recorteActivado;
}

void Tripleta2::setEtapas (Etapas e){
    etapa = e;
}

Etapas Tripleta2::getEtapas (){
    return etapa;
}

QVector <cinterval> * Tripleta2::getTerNume() {
    return terNume;
}

void Tripleta2::setTerNume (QVector <cinterval> * terNume){
    this->terNume = terNume;
}

QVector <cinterval> * Tripleta2::getTerDeno() {
    return terDeno;
}

void Tripleta2::setTerDeno (QVector <cinterval> * terDeno){
    this->terDeno = terDeno;
}

cinterval Tripleta2::getTerK() {
    return terK;
}

void Tripleta2::setTerK (cinterval terK){
    this->terK = terK;
}

void Tripleta2::setLados (qreal m, qreal f){
    anchoFas = f;
    anchoMag = m;
}

qreal Tripleta2::getAnchoFas () {
    return anchoFas;
}

qreal Tripleta2::getAnchoMag () {
    return anchoMag;
}

QVector <ListaOrdenada *> * Tripleta2::getNumeradorInfMag () {
    return numeradorInfMag;
}

void Tripleta2::setNumeradorInfMag (QVector <ListaOrdenada *> * m) {
    numeradorInfMag = m;
}

QVector <ListaOrdenada *> * Tripleta2::getNumeradorInfFas () {
    return numeradorInfFas;
}

void Tripleta2::setNumeradorInfFas (QVector <ListaOrdenada *> * m) {
    numeradorInfFas = m;
}

QVector <ListaOrdenada *> * Tripleta2::getNumeradorSupMag () {
    return numeradorSupMag;
}

void Tripleta2::setNumeradorSupMag (QVector <ListaOrdenada *> * m) {
    numeradorSupMag = m;
}

QVector <ListaOrdenada *> * Tripleta2::getNumeradorSupFas () {
    return numeradorSupFas;
}

void Tripleta2::setNumeradorSupFas (QVector <ListaOrdenada *> * m) {
    numeradorSupFas = m;
}

QVector <ListaOrdenada *> * Tripleta2::getDenominadorInfMag () {
    return denominadorInfMag;
}

void Tripleta2::setDenominadorInfMag (QVector <ListaOrdenada *> * m) {
    denominadorInfMag = m;
}

QVector <ListaOrdenada *> * Tripleta2::getDenominadorInfFas () {
    return denominadorInfFas;
}

void Tripleta2::setDenominadorInfFas (QVector <ListaOrdenada *> * m) {
    denominadorInfFas = m;
}

QVector <ListaOrdenada *> * Tripleta2::getDenominadorSupMag () {
    return denominadorSupMag;
}

void Tripleta2::setDenominadorSupMag (QVector <ListaOrdenada *> * m) {
    denominadorSupMag = m;
}

QVector <ListaOrdenada *> * Tripleta2::getDenominadorSupFas () {
    return denominadorSupFas;
}

void Tripleta2::setDenominadorSupFas (QVector <ListaOrdenada *> * m) {
    denominadorSupFas = m;
}

ListaOrdenada * Tripleta2::getKInf () {
    return kInf;
}

void Tripleta2::setKInf (ListaOrdenada * lista) {
    kInf = lista;
}

ListaOrdenada * Tripleta2::getKSup () {
    return kSup;
}

void Tripleta2::setKSup (ListaOrdenada * lista) {
    kSup = lista;
}

QVector<data_box *> *Tripleta2::getDatosCortesBoundaries(){
    return datosCortesBoundaries;
}

void Tripleta2::setDatosCortesBoundaries(QVector<data_box *> *datos) {

    if (datosCortesBoundaries != nullptr) {
        datosCortesBoundaries->clear();
    }

    datosCortesBoundaries = datos;
}

void Tripleta2::addFrecuenciaFeasible(qreal pos, qreal frec) {

    if (frecuenciasFeasible == nullptr) {
        frecuenciasFeasible = new QHash <qreal, qreal> ();
    }

    frecuenciasFeasible->insert(pos, frec);
}

bool Tripleta2::isFrecueciaFeasible(qreal key) {
    return frecuenciasFeasible == nullptr ? false : frecuenciasFeasible->contains(key);
}

qreal Tripleta2::getFrecuenciaFeasible(qreal key) {
    return frecuenciasFeasible == nullptr ? -1 : frecuenciasFeasible->value(key);
}

qreal Tripleta2::getPosFrecuenciaFeasible(qreal value) {
    return frecuenciasFeasible == nullptr ? -1 : frecuenciasFeasible->key(value);
}

void Tripleta2::setFrecuenciasFeasible(QHash<qreal, qreal> *frecuenciasFeasible) {
    this->frecuenciasFeasible = frecuenciasFeasible;
}

QHash <qreal, qreal> * Tripleta2::getFrecuenciasFeasible() {
    return frecuenciasFeasible;
}
