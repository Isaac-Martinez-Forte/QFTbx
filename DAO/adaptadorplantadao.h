#ifndef ADAPTADORPLANTADAO_H
#define ADAPTADORPLANTADAO_H

#include "src/core/system/lti_system.h"
#include "DAO/plantadao.h"

/**
 * @class AdaptadorPlantaDAO
 * @brief Clase que implementa el patrón DAO para guardar en memoria una planta definida por el usuario
 * 
 * @author Isaac Martínez Forte
 */

class AdaptadorPlantaDAO : public PlantaDAO
{
public:

  /**
    * @fn AdaptadorPlantaDAO
    * @brief Constructor de la clase.
   */

    AdaptadorPlantaDAO();
    ~AdaptadorPlantaDAO();


  /**
    * @fn getPlanta
    * @brief Retorna la planta definida en el objeto, si es vacia retornará un puntero a NULL.
    * @return Planta guardada en el objeto.
   */

    LtiSystem * getPlanta ();


   /**
    * @fn setPlanta
    * @brief Guarda la planta pasada por parámetro.
    * 
    * Si ya existe una planta guardada enteriormente destruirá dicha planta para que no haya fugas de memoria.
    * 
    * @param planta a guardar.
   */

    void setPlanta (LtiSystem * planta);

private:
    LtiSystem * planta = NULL;

};

#endif // ADAPTADORPLANTADAO_H
