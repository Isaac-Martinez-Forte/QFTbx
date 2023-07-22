#include "datosbound.h"

DatosBound::DatosBound(QVector <QMap <QString, QVector <QVector <QPointF> * > *> * > * boundaries,
                           QVector <bool> * metadatosabierta, QVector <bool> * metadatosarriba,
                           qint32 tamFas, QPointF datosFas, QVector< QVector<QPointF> * > * boundaries_reun,
                           QVector< QVector< QVector<QPointF> * > * > * hash_inter_boundaries, qint32 tamMag, QPointF DatosMag)
{
    this->boundaries = boundaries;
    this->datosFas = datosFas;
    this->DatosMag = DatosMag;
    this->tamFas = tamFas;
    this->tamMag = tamMag;
    this->boundaries_reun = boundaries_reun;
    this->hash_inter_boundaries = hash_inter_boundaries;
    this->metadatosabierta = metadatosabierta;
    this->metadatosarriba = metadatosarriba;
}

DatosBound::~DatosBound(){
    boundaries->clear();
    hash_inter_boundaries->clear();
    metadatosabierta->clear();
    metadatosarriba->clear();
    boundaries_reun->clear();

    if (datosFasBound != NULL){
        datosFasBound->clear();
    }

    if (datosMagBound != NULL) {
        datosMagBound->clear();
    }

    if (datosFasBoundLin != NULL){
        datosFasBoundLin->clear();
    }

    if (datosMagBoundLin != NULL) {
        datosMagBoundLin->clear();
    }
}

void DatosBound::setBox(cxsc::cinterval a) {
    box = a;
}

cxsc::cinterval DatosBound::getBox(){
    return box;
}

QVector<QMap<QString, QVector<QVector<QPointF> *> *> *> *DatosBound::getBoundaries(){
    return boundaries;
}

qint32 DatosBound::getTamFas(){
    return tamFas;
}

qint32 DatosBound::getTamMag(){
    return tamMag;
}

QPointF DatosBound::getDatosFas(){
    return datosFas;
}

QPointF DatosBound::getDatosMag(){
    return DatosMag;
}

QVector<QVector<QPointF> *> *DatosBound::getBoundariesReun(){
    return boundaries_reun;
}

QVector<QVector<QVector<QPointF> *> *> *DatosBound::getBoundariesReunHash(){
    return hash_inter_boundaries;
}

QVector <bool> * DatosBound::getMetaDatosAbierta(){
    return metadatosabierta;
}

QVector <bool> * DatosBound::getMetaDatosArriba(){
    return metadatosarriba;
}

QVector<QPointF> * DatosBound::getDatosFasBound() {
    return datosFasBound;
}

QVector<QPointF> * DatosBound::getDatosMagBound() {
    return datosMagBound;
}

void DatosBound::setDatosFasBound(QVector<QPointF> *v){
    datosFasBound = v;
}

void DatosBound::setDatosMagBound(QVector<QPointF> *v) {
    datosMagBound = v;
}

QVector<QPointF> * DatosBound::getDatosFasBoundLin() {
    return datosFasBoundLin;
}

QVector<QPointF> * DatosBound::getDatosMagBoundLin() {
    return datosMagBoundLin;
}

void DatosBound::setDatosFasBoundLin(QVector<QPointF> *v){
    datosFasBoundLin = v;
}

void DatosBound::setDatosMagBoundLin(QVector<QPointF> *v) {
    datosMagBoundLin = v;
}
