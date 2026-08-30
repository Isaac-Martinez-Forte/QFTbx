#include "omega.h"

#include <QFile>
#include <QTextStream>

#include "../Herramientas/exception.h"
#include "../Herramientas/tools.h"

using namespace tools;

Omega::Omega(qreal inicio, qreal final, qint32 nPuntos, QVector<qreal> *valores, tiposOmega tipo)
{
    if (valores == NULL || valores->isEmpty()){
        delete valores;
        throw qftbx::InvalidInput("A frequency set needs at least one value.");
    }

    this->inicio = inicio;
    this->final = final;
    //Invariante: nPuntos == valores->size() siempre. El parametro se ignora
    //deliberadamente: ficheros antiguos traen un <nPuntos> desincronizado.
    this->nPuntos = valores->size();
    this->valores = valores;
    this->tipo = tipo;
}

Omega::~Omega(){
    delete valores;
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

    if (nueva_omega == NULL || nueva_omega->isEmpty()){
        if (nueva_omega != valores){
            delete nueva_omega;
        }
        throw qftbx::InvalidInput("A frequency set needs at least one value.");
    }

    //Templates/Boundaries a veces devuelven el mismo vector que ya poseemos.
    if (nueva_omega == valores){
        nPuntos = valores->size();
        return;
    }

    delete valores;
    nPuntos = nueva_omega->size();
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
