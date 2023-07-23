#ifndef CONTROLADORDAO_H
#define CONTROLADORDAO_H

#include <Modelo/EstructuraSistema/sistema.h>

  /**
    * @class ControladorDAO
    * @brief Clase que implementa el patrón DAO para guardar un Controlador en el sistema.
    *
    * Esta clase es abstracta dado que contiene dos funciones virtuales puras, para que sea redefinidas por sus hijos y así implementar adapatadores del patrón DAO.
    *
    * @author Isaac Martínez Forte
   */


class ControladorDAO
{
public:

  /**
    * @fn ~ConroladorDAO
    * @brief Destructor de la clase, solo puede ser llamado desde un hijo.
    *
    */

    virtual ~ControladorDAO() {}

  /**
    * @fn getControlador
    * @brief Clase virtual pura cuya redefinición deberá devolver un Controlador guardado en el sistema.
    *
    * @return Función de transferencia con el controlador guardado en el sistema.
    */

    virtual std::shared_ptr<Sistema> getControlador () = 0;

  /**
    * @fn setControlador
    * @brief Clase virtual pura cuya redefinición deberá guardar un Controlador en el sistema.
    *
    * @param Función de transferencia a guardar en el sistema.
    */

    virtual void setControlador (std::shared_ptr<Sistema> controlador) = 0;
};

#endif // CONTROLADORDAO_H
