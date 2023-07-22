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

    virtual PlantaDAO * getPlantaDAO() = 0;


    /**
      * @fn getOmegaDAO()
      * @brief Función virtual pura que retorna el AdaptadorOmegaDAO.
      *
      * @return OmegaDAO con una instancia de AdaptadorOmegaDAO.
     */

    virtual OmegaDAO * getOmegaDAO() = 0;


    /**
      * @fn getTemplateDAO
      * @brief Función virtual pura que retorna el AdaptadorTemplateDAO.
      *
      * @return TemplateDAO con una instancia de AdaptadorTemplateDAO
     */

    virtual TemplateDAO * getTemplateDAO() = 0;


    /**
      * @fn getBoundDAO()
      * @brief Función virtual pura que retorna el adaptadorBoundDAO
      *
      * @return BoundDAO con una instancia de AdaptadorBoundDAO
     */

    virtual BoundDAO * getBoundDAO() = 0;


    virtual EspecificacionesDAO * getEspecificacionesDAO() = 0;

    virtual ControladorDAO * getControladorDAO() = 0;

    virtual LoopShapingDAO * getLoopShapingDAO() = 0;

};

#endif // DAO_H
