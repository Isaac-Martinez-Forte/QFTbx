#ifndef DATOSFEASIBLE_H
#define DATOSFEASIBLE_H

#include "n.h"

class DatosFeasible : public N {

public:

    DatosFeasible (qreal index, qreal indice, QString variable, qreal porcentaje);

    qreal getOmega ();

    void setOmega (qreal indice);

    QString getName ();

    void setName (QString name);

    qreal getPorcentaje ();

    void setPorcentaje (qreal porcentaje);

private:

    qreal omega;
    QString name;
    qreal porcentaje;

};

#endif // DATOSFEASIBLE_H
