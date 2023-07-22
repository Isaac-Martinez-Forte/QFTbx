#ifndef POLOSCEROS_H
#define POLOSCEROS_H

#include "funciontransferencia.h"
#include "muparserx/mpParser.h"


  /**
    * @class PolosCeros
    * @brief Clase que representa una especialización de Planta en este caso especialización de FuncionTransferencia.
    * 
    * Esta clase es abstracta por lo tanto no puede ser instanciable, sirve para agrupar los distintos tipos de plantas definidas con Polos y Ceros.
    * 
    * @author Isaac Martínez Forte
   */

class PolosCeros : public FuncionTransferencia
{
public:
  
  
  /**
    * @fn PolosCeros
    * @brief Constructor de la clase que tiene como parámetros los datos básicos de una planta de este tipo.
    * 
    * @param nombre de la planta.
    * @param numerador de la planta
    * @param denominador de la planta
    * @param k ganancia asociada a la planta
    * @param retardo asociado a la planta
   */
  
    PolosCeros(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret);
    
    
  /**
    * @fn PolosCeros
    * @brief Constructor de la clase que tiene como parámetro un objeto ya creado del mismo tipo.
    * 
    * @param datos objeto del mismo tipo creado anteriormente.
    */
    
    PolosCeros(PolosCeros * datos);

    virtual Sistema * invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret = NULL, QString exp_nume = 0, QString exp_deno = 0) = 0;
    
    
    /**
     * @fn ~PolosCeros
     * @brief Destructor de la clase.
     */
    
    ~PolosCeros();

    virtual QString getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega) = 0;

    virtual QString getExpr(qreal w) = 0;

    virtual QString getExpr() = 0;

    virtual std::complex <qreal> getPuntoNume(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> getPuntoDeno(QVector <qreal> * deno, qreal omega) = 0;
    
   /**
    * @fn getClass
    * @brief Función virtual pura que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    virtual tipo_planta getClass() = 0;

};

#endif // POLOSCEROS_H
