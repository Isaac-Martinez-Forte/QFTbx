#ifndef DAO_H
#define DAO_H

#include "bounddao.h"
#include "omegadao.h"
#include "plantadao.h"
#include "templatedao.h"
#include "especificacionesdao.h"
#include "controladordao.h"
#include "loopshapingdao.h"

/**
 * @class DAO
 * @brief Clase que define la factoría patrón DAO con las funciones recuperar los adaptadores concretos.
 * @author Isaac Martínez Forte
 */

class DAO
{
public:


    /**
      * @fn ~DAO
      * @brief Destructor de la clase.
     */

    virtual ~DAO() {}


    /**
      * @fn getPlantaDAO
      * @brief Función virtual pura que retorna el AdaptadorPlantaDAO.
      *
      * @return PlantaDAO con una instancia de AdaptadorPlantaDAO.
     */

    virtual std::shared_ptr<PlantaDAO> getPlantaDAO() = 0;


    /**
      * @fn getOmegaDAO()
      * @brief Función virtual pura que retorna el AdaptadorOmegaDAO.
      *
      * @return OmegaDAO con una instancia de AdaptadorOmegaDAO.
     */

    virtual std::shared_ptr<OmegaDAO> getOmegaDAO() = 0;


    /**
      * @fn getTemplateDAO
      * @brief Función virtual pura que retorna el AdaptadorTemplateDAO.
      *
      * @return TemplateDAO con una instancia de AdaptadorTemplateDAO
     */

    virtual std::shared_ptr<TemplateDAO> getTemplateDAO() = 0;


    /**
      * @fn getBoundDAO()
      * @brief Función virtual pura que retorna el adaptadorBoundDAO
      *
      * @return BoundDAO con una instancia de AdaptadorBoundDAO
     */

    virtual std::shared_ptr<BoundDAO> getBoundDAO() = 0;


    virtual std::shared_ptr<EspecificacionesDAO> getEspecificacionesDAO() = 0;

    virtual std::shared_ptr<ControladorDAO> getControladorDAO() = 0;

    virtual std::shared_ptr<LoopShapingDAO> getLoopShapingDAO() = 0;

};

#endif // DAO_H
