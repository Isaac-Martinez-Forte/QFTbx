#include "omega.h"

#include <QFile>
#include <QTextStream>

#include "../Herramientas/exception.h"
#include "../Herramientas/tools.h"

using namespace tools;

Omega::Omega(qreal inicio, qreal final, qint32 nPuntos, QVector<qreal> *valores, tiposOmega tipo)
{
    this->inicio = inicio;
    this->final = final;
    this->nPuntos = nPuntos;
    this->valores = valores;
    this->tipo = tipo;
}

Omega::~Omega(){
    valores->clear();
}

qreal Omega::getInicio(){
    return inicio;
}

qreal Omega::getFinal(){
    return final;
}

qint32 Omega::getNPuntos(){
    return nPuntos;
}

QVector<qreal> * Omega::getValores(){
    return valores;
}

Omega::tiposOmega Omega::getTipo(){
    return tipo;
}

void Omega::setOmega(QVector<qreal> * nueva_omega){
  //  valores->clear();
    valores = nueva_omega;
}

QVector <qreal> * Omega::valuesFromFile(QString path){

    QFile file (path);

    if (!file.open(QIODevice::ReadOnly)){
        throw qftbx::FileError("Cannot open frequencies file: " + path.toStdString());
    }

    QTextStream in (&file);
    QVector <qreal> * values = tools::srtovectorReal(in.readAll());

    if (values == NULL || values->isEmpty()){
        delete values;
        throw qftbx::FileError("The frequencies file contains no valid values: "
                               + path.toStdString());
    }

    return values;
}
