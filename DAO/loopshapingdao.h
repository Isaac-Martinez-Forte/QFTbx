#ifndef LOOPSHAPINGDAO
#define LOOPSHAPINGDAO

#include <Modelo/EstructurasDatos/datosloopshaping.h>

class LoopShapingDAO
{
public:

    virtual ~LoopShapingDAO() {}

    virtual std::shared_ptr<DatosLoopShaping> getLoopShaping () = 0;

    virtual void setDatos (std::shared_ptr<DatosLoopShaping> datos) = 0;

};

#endif // LOOPSHAPINGDAO

