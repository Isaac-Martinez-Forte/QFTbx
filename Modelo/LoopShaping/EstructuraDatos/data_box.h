#ifndef DATA_BOX_H
#define DATA_BOX_H

#include "Modelo/Herramientas/tools.h"
#include "QVector"

class data_box
{
public:
    data_box();

    ~data_box();

    void setFlag (tools::flags_box f);
    tools::flags_box getFlag();

    void setMinimoxMaximos(QVector <qreal> * mm);
    QVector <qreal> * getMinimoxMaximos ();

    void setCompleto (bool c);
    bool isCompleto ();

    void setRecArriba (bool r);
    bool isRecArriba();

    void setRecAbajo (bool r);
    bool isRecAbajo();

    void setUniArriba (bool r);
    bool isUniArriba();

    void setUniAbajo (bool r);
    bool isUniAbajo();

    void setRecDerecha (bool r);
    bool isRecDerecha();

    void setRecIzquierda (bool r);
    bool isRecIzquierda();

    void setUniDerecha (bool r);
    bool isUniDerecha();

    void setUniIzquierda (bool r);
    bool isUniIzquierda();

    void setCambioEtapa (bool r);
    bool getCambioEtapa();


private:

    tools::flags_box flag;
    QVector <qreal> * minimosMaximos = nullptr;
    qreal porcentajeFeasible = 0;

    bool completo = false;

    // Se puede recortar por arriba, y arriba está la parte infeasible.
    bool recArriba= false, uniArriba= false;

    // Se puede recortar por abajo, y abajo está la parte infeasible.
    bool recAbajo= false, uniAbajo= false;

    // ...
    bool recDerecha= false, uniDerecha= false;
    bool recIzquierda= false, uniIzquierda= false;

    bool cambioEtapa = false;

};

#endif // DATA_BOX_H
