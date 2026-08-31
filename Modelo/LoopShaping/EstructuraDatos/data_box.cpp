#include "data_box.h"

data_box::data_box()
{
}

data_box::~data_box()
{
    delete minimosMaximos;
}

void data_box::setFlag(tools::flags_box f)
{
    flag = f;
}

tools::flags_box data_box::getFlag()
{
    return flag;
}

void data_box::setMinimoxMaximos(QVector<qreal> * mm)
{
    delete minimosMaximos;
    minimosMaximos = mm;
}

QVector<qreal> * data_box::getMinimoxMaximos()
{
    return minimosMaximos;
}

void data_box::setUniArriba(bool r)
{
    uniArriba = r;
}

bool data_box::isUniArriba()
{
    return uniArriba;
}

void data_box::setUniAbajo(bool r)
{
    uniAbajo = r;
}

bool data_box::isUniAbajo()
{
    return uniAbajo;
}

void data_box::setUniDerecha(bool r)
{
    uniDerecha = r;
}

bool data_box::isUniDerecha()
{
    return uniDerecha;
}

void data_box::setUniIzquierda(bool r)
{
    uniIzquierda = r;
}

bool data_box::isUniIzquierda()
{
    return uniIzquierda;
}
