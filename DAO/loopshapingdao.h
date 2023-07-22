#ifndef LOOPSHAPINGDAO
#define LOOPSHAPINGDAO

#include <Modelo/EstructurasDatos/datosloopshaping.h>

class LoopShapingDAO
{
public:

    virtual ~LoopShapingDAO() {}

    virtual DatosLoopShaping * getLoopShaping () = 0;

    virtual void setDatos (DatosLoopShaping * datos) = 0;

};

#endif // LOOPSHAPINGDAO

