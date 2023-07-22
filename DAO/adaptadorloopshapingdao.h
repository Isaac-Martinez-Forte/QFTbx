#ifndef ADAPTADORLOOPSHAPINGDAO_H
#define ADAPTADORLOOPSHAPINGDAO_H

#include "loopshapingdao.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"

class AdaptadorLoopShapingDAO : public LoopShapingDAO
{
public:

    AdaptadorLoopShapingDAO();
    ~AdaptadorLoopShapingDAO();

    DatosLoopShaping * getLoopShaping ();

    void setDatos (DatosLoopShaping * datos);

private:

    DatosLoopShaping * datos;

    bool introducido;
};

#endif // ADAPTADORLOOPSHAPINGDAO_H
