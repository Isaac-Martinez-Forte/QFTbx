#include "sistema.h"

using namespace std;

Sistema::Sistema(QString nombre)
{
    this->nombre = nombre;
}

void Sistema:: setNombre (QString nombre){
    this->nombre = nombre;
}

QString Sistema:: getNombre(){
    return nombre;
}


