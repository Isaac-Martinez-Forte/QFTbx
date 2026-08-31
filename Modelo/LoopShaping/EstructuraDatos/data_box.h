#ifndef DATA_BOX_H
#define DATA_BOX_H

#include <QVector>

#include "Modelo/Herramientas/tools.h"

//Result of classifying one projected Nichols box against the boundary
//union at one design frequency (DeteccionViolacionBoundaries): the
//feasibility flag, the boundary extremes over the box's phase span
//(B_min/B_max in dB, C_min/C_max in degrees, indices 0-3 of
//minimosMaximos) and the corner classifications that certify the cutting
//strips (uniAbajo/uniIzquierda: bottom-left corner infeasible;
//uniDerecha: top-right corner infeasible; uniArriba keeps the historical
//meaning "bottom-left corner feasible" for algorithm NK's gate).
class data_box
{
public:
    data_box();

    ~data_box();

    void setFlag(tools::flags_box f);
    tools::flags_box getFlag();

    void setMinimoxMaximos(QVector<qreal> * mm);
    QVector<qreal> * getMinimoxMaximos();

    void setUniArriba(bool r);
    bool isUniArriba();

    void setUniAbajo(bool r);
    bool isUniAbajo();

    void setUniDerecha(bool r);
    bool isUniDerecha();

    void setUniIzquierda(bool r);
    bool isUniIzquierda();

private:

    tools::flags_box flag;
    QVector<qreal> * minimosMaximos = nullptr;

    bool uniArriba = false;
    bool uniAbajo = false;
    bool uniDerecha = false;
    bool uniIzquierda = false;
};

#endif // DATA_BOX_H
