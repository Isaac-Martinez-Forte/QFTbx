#include "tripleta2.h"

using namespace tools;

Tripleta2::Tripleta2(qreal index, LtiSystem * sistema, flags_box flags)
    : Tripleta(index, sistema, flags)
{
}

Tripleta2::~Tripleta2()
{
    delete frecuenciasFeasible;
}

void Tripleta2::setRecorteActivado(bool recorteActivado)
{
    this->recorteActivado = recorteActivado;
}

bool Tripleta2::isRecorteActivado()
{
    return recorteActivado;
}

void Tripleta2::setEtapas(Etapas e)
{
    etapa = e;
}

Etapas Tripleta2::getEtapas()
{
    return etapa;
}

void Tripleta2::addFrecuenciaFeasible(qreal pos, qreal frec)
{
    if (frecuenciasFeasible == nullptr) {
        frecuenciasFeasible = new QHash<qreal, qreal>();
    }

    frecuenciasFeasible->insert(pos, frec);
}

bool Tripleta2::isFrecueciaFeasible(qreal key)
{
    return frecuenciasFeasible != nullptr && frecuenciasFeasible->contains(key);
}

void Tripleta2::setFrecuenciasFeasible(QHash<qreal, qreal> * frecuenciasFeasible)
{
    delete this->frecuenciasFeasible;
    this->frecuenciasFeasible = frecuenciasFeasible;
}

QHash<qreal, qreal> * Tripleta2::getFrecuenciasFeasible()
{
    return frecuenciasFeasible;
}
