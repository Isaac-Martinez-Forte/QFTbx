#ifndef ADAPTADORLOOPSHAPINGDAO_H
#define ADAPTADORLOOPSHAPINGDAO_H

#include "loopshapingdao.h"
#include "Modelo/EstructurasDatos/datosloopshaping.h"

class AdaptadorLoopShapingDAO : public LoopShapingDAO
{
public:

    AdaptadorLoopShapingDAO();
    ~AdaptadorLoopShapingDAO();

    std::shared_ptr<DatosLoopShaping> getLoopShaping ();

    void setDatos (std::shared_ptr<DatosLoopShaping> datos);

private:
    std::shared_ptr<DatosLoopShaping> datos;

};

#endif // ADAPTADORLOOPSHAPINGDAO_H
