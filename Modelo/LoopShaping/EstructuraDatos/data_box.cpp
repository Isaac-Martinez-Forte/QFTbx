#include "data_box.h"

data_box::data_box()
{

}

data_box::~data_box() {
    if (minimosMaximos != nullptr) {
        minimosMaximos->clear();
    }
}

void data_box::setFlag (tools::flags_box f){
    flag = f;
}
tools::flags_box data_box::getFlag(){
    return flag;
}

void data_box::setMinimoxMaximos(QVector <qreal> * mm){
    minimosMaximos = mm;
}

QVector <qreal> * data_box::getMinimoxMaximos (){
    return minimosMaximos;
}

void data_box::setCompleto (bool c){
    completo = c;
}

bool data_box::isCompleto (){
    return completo;
}

void data_box::setRecArriba (bool r){
    recArriba = r;
}

bool data_box::isRecArriba(){
    return recArriba;
}

void data_box::setRecAbajo (bool r){
    recAbajo = r;
}

bool data_box::isRecAbajo(){
    return recAbajo;
}

void data_box::setUniArriba (bool r){
    uniArriba = r;
}

bool data_box::isUniArriba(){
    return uniArriba;
}

void data_box::setUniAbajo (bool r){
    uniAbajo = r;
}

bool data_box::isUniAbajo() {
    return uniAbajo;
}

void data_box::setRecDerecha (bool r){
    recDerecha = r;
}

bool data_box::isRecDerecha(){
    return recDerecha;
}

void data_box::setRecIzquierda (bool r){
    recIzquierda = r;
}

bool data_box::isRecIzquierda(){
    return recIzquierda;
}

void data_box::setUniDerecha (bool r){
    uniDerecha = r;
}

bool data_box::isUniDerecha(){
    return uniDerecha;
}

void data_box::setUniIzquierda (bool r){
    uniIzquierda = r;
}

bool data_box::isUniIzquierda(){
    return uniIzquierda;
}

void data_box::setCambioEtapa(bool r) {
    cambioEtapa = r;
}

bool data_box::getCambioEtapa() {
    return cambioEtapa;
}
