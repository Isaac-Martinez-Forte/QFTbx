#ifndef FDAO_H
#define FDAO_H

#include "dao.h"
#include "bounddao.h"
#include "omegadao.h"
#include "plantadao.h"
#include "templatedao.h"
#include "especificacionesdao.h"
#include "adaptadorbounddao.h"
#include "adaptadoromegadao.h"
#include "adaptadorplantadao.h"
#include "adaptadortemplatedao.h"
#include "adaptadorespecificacionesdao.h"
#include "adaptadorcontroladordao.h"
#include "loopshapingdao.h"
#include "adaptadorloopshapingdao.h"


/**
 * @class FDAO
 * @brief Clase que implementa la factoría del patrón DAO con las funciones recuperar los adaptadores concretos.
 * @author Isaac Martínez Forte
 */

class FDAO : public DAO
{
public:
    FDAO();


    /**
      * @fn getPlantaDAO
      * @brief Función que retorna el AdaptadorPlantaDAO.
      *
      * @return PlantaDAO con una instancia de AdaptadorPlantaDAO.
     */

    ~FDAO();

    std::shared_ptr<PlantaDAO> getPlantaDAO();


    /**
      * @fn getOmegaDAO()
      * @brief Función que retorna el AdaptadorOmegaDAO.
      *
      * @return OmegaDAO con una instancia de AdaptadorOmegaDAO.
     */

    std::shared_ptr<OmegaDAO> getOmegaDAO();


    /**
      * @fn getTemplateDAO
      * @brief Función que retorna el AdaptadorTemplateDAO.
      *
      * @return TemplateDAO con una instancia de AdaptadorTemplateDAO
     */

    std::shared_ptr<TemplateDAO> getTemplateDAO();


    /**
      * @fn getBoundDAO()
      * @brief Función que retorna el adaptadorBoundDAO
      *
      * @return BoundDAO con una instancia de AdaptadorBoundDAO
     */

    std::shared_ptr<BoundDAO> getBoundDAO();


    std::shared_ptr<EspecificacionesDAO> getEspecificacionesDAO();

    std::shared_ptr<ControladorDAO> getControladorDAO();

    std::shared_ptr<LoopShapingDAO> getLoopShapingDAO();

};

#endif // FDAO_H
