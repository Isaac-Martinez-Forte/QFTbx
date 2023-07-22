#ifndef DATOSFEASIBLE_H
#define DATOSFEASIBLE_H

#include "n.h"

class DatosFeasible : public N {

public:

    DatosFeasible (qreal index, qreal posFrecuencia, QString variable, qreal porcentaje);

    qreal getPosFrecuencia ();

    void setPosFrecuencia (qreal posFrecuencia);

    QString getName ();

    void setName (QString name);

    qreal getPorcentaje ();

    void setPorcentaje (qreal porcentaje);

private:

    qreal posFrecuencia;
    QString name;
    qreal porcentaje;

};

#endif // DATOSFEASIBLE_H
