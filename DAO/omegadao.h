#ifndef OMEGADAO_H
#define OMEGADAO_H

#include <QVector>

#include "Modelo/Objetos/omega.h"


  /**
    * @class OmegaDAO
    * @brief Clase que gestiona, aplicando el patrón DAO, el guardado de frecuencias de diseño (Omega) en el sistema.
    * 
    * @author Isaac Martínez Forte
   */

class OmegaDAO
{
public:
  
  /**
    * @fn ~OmegaDAO
    * @brief Destructor que crea el objeto, pero al ser una clase abstracta esta clase no se puede instanciar, pero este destructor puede ser llamado desde una clase hija.
    * 
   */
  
    virtual ~OmegaDAO() {}

  /**
    * @fn setFrecuencias
    * @brief Función virtual pura que guarda un vector de frecuencias en el sistema, aparte de otros datos asociados.
    * Está pensada para que sea implementada por sus clases hijas.
    * 
    * @param omega Frecuencias de diseño.
   */
  
    
    virtual void setOmega(Omega * omega) = 0;
    
  /**
    * @fn getFrecuencias
    * @brief Función virtual pura que devuelve un vector con las frecuencias guardadas en el sistema.
    * Está pensada para que sea implementada por sus hijos.
    * 
    * @return vector con las frecuencias Omega guardadas en el objeto.
   */
    
    virtual QVector<qreal> * getFrecuencias () = 0;

    /**
      * @fn getOmega
      * @brief Función virtual pura que devuelve una clase Omega con todos los datos de las frecuencias Omega introducidas en el sistema.
      * Está pensada para que sea implementada por sus hijos.
      *
      * @return Omega frecuencias de diseño.
     */

    virtual Omega * getOmega () = 0;

};

#endif // OMEGADAO_H
