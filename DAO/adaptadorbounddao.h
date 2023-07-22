#ifndef ADAPTADORBOUNDDAO_H
#define ADAPTADORBOUNDDAO_H

#include "DAO/bounddao.h"

/**
 * @class AdaptadorBoundDAO
 * @brief Clase que implementa el patrón DAO para guardar en memoria los boundaries calculados y la sábana paso intermedio del cálculo.
 * @author Isaac Martínez Forte
 */

class AdaptadorBoundDAO : public BoundDAO
{
public:

    /**
      * @fn AdaptadorBoundDAO
      * @brief Constructor que crea el objeto AdaptadorBoundDAO.
     */

    AdaptadorBoundDAO();
    ~AdaptadorBoundDAO();

    /**
     * @fn setBound
     * @brief Guarda los boundaries calculados así como la sábana del cálculo intermedio.
     *
     * Si hubiera datos guardados anteriormente estos serán destruidos para que no haya fugas de memoria.
     *
     * @param boundaries calculados.
     *
     */

    void setBound(DatosBound * boundaries);

    /**
    * @fn getBound
    * @brief Retorna los boundaries guardados
    *
    * @return boundaries guardados en el sistema.
    *
    */

    DatosBound * getBound();


private:
    bool introducido;
    DatosBound * bound;

};

#endif // ADAPTADORBOUNDDAO_H
