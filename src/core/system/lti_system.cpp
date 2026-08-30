#include "lti_system.h"

using namespace std;

LtiSystem::LtiSystem(QString nombre)
{
    this->nombre = nombre;
}

void LtiSystem:: setName (QString nombre){
    this->nombre = nombre;
}

QString LtiSystem:: name(){
    return nombre;
}


