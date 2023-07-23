#ifndef BOUNDDAO_H
#define BOUNDDAO_H

#include <QVector>
#include <QMap>
#include <QString>
#include <memory>

#include "Modelo/EstructurasDatos/datosbound.h"

/**
 * @class BoundDAO
 * @brief Clase virtual que define el patrón DAO con las funciones para guardar los Boundaries
 * @author Isaac Martínez Forte
 */


class BoundDAO
{
public:

    /**
      * @fn ~BoundDAO
      * @brief Destructor de la clase.
     */

    virtual ~BoundDAO(){}

    /**
      * @fn setBound
      * @brief Función virtual que tiene el objetivo de guardar los boundaries y la sábanas de cálculo intermedio.
      *
      * @param boundaries a guardar en el sistema.
     */

    virtual void setBound(std::shared_ptr<DatosBound> boundaries) = 0;


    /**
      * @fn getBound
      * @brief Función virtual que tiene el objetivo de retornar los boundaries guardados.
      *
      * @return boundaries guardados en la clase.
     */

    virtual std::shared_ptr<DatosBound> getBound() = 0;
};

#endif // BOUNDDAO_H
