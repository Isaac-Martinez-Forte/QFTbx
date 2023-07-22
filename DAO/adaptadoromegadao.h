#ifndef ADAPTADOROMEGADAO_H
#define ADAPTADOROMEGADAO_H

#include <DAO/omegadao.h>
#include <QVector>

#include "Modelo/Objetos/omega.h"

/**
 * @class AdaptadorOmegaDAO
 * @brief Clase que implementa el patrón DAO para guardar en memoria el vector de frecuencias Omega y otros datos asociados.
 * 
 * @author Isaac Martínez Forte
 */

class AdaptadorOmegaDAO : public OmegaDAO
{
public:
  
  /**
    * @fn AdaptadorOmegaDAO
    * @brief Constructor que crea el objeto AdaptadorOmegaDAO.
   */
  
    AdaptadorOmegaDAO();
    ~AdaptadorOmegaDAO();

   /**
    * @fn setOmega
    * @brief Guarda un vector de frecuencias en el objeto y otros datos asociados encapsulados en el objeto Omega.
    * 
    * Si hubiera datos guardados anteriormente estos serán destruidos para que no haya fugas de memoria.
    * 
    * @param omega con los datos de las frecuencias de diseño.
    * 
    */
  
    void setOmega(Omega * omega);
    
    /**
    * @fn getFrecuencias
    * @brief Retorna el vector de frecuencias guardado en el objeto.
    * 
    * @return vector con las frecuencias de diseño.
    * 
    */
    
    QVector<qreal> * getFrecuencias();

    /**
    * @fn getOmega
    * @brief Retorna el objeto Omega que guarda el vector de frecuencias de diseño y otros datos asociados.
    *
    * @return Objeto Omega.
    *
    */

    Omega * getOmega();

private:
    Omega * omega = NULL;
};

#endif // ADAPTADOROMEGADAO_H
