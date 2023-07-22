#ifndef PLANTADAO_H
#define PLANTADAO_H

#include <Modelo/EstructuraSistema/sistema.h>

  /**
    * @class PlantaDAO
    * @brief Clase que implementa el patrón DAO para guardar una Planta en el sistema.
    * 
    * Esta clase es abstracta dado que contiene dos funciones virtuales puras, para que sea redefinidas por sus hijos y así implementar adapatadores del patrón DAO.
    * 
    * @author Isaac Martínez Forte
   */


class PlantaDAO
{
public:
  
  /**
    * @fn ~PlantaDAO
    * @brief Destructor de la clase, solo puede ser llamado desde un hijo.
    * 
    */

    virtual ~PlantaDAO() {}
    
  /**
    * @fn getPlanta
    * @brief Clase virtual pura cuya redefinición deberá devolver una Planta guardada en el sistema.
    *
    * @return Planta con la planta guardada en el sistema.
    */

    virtual Sistema * getPlanta () = 0;
    
  /**
    * @fn setPlanta
    * @brief Clase virtual pura cuya redefinición deberá guardar una Planta en el sistema.
    * 
    * @param planta a guardar en el sistema.
    */
    
    virtual void setPlanta (Sistema * planta) = 0;
};

#endif // PLANTADAO_H
