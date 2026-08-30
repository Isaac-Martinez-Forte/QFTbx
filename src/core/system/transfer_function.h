#ifndef QFTBX_TRANSFER_FUNCTION_H
#define QFTBX_TRANSFER_FUNCTION_H

#include "lti_system.h"
#include <QVector>
#include "src/core/system/parameter.h"
#include "mpParser.h"

 /**
    * @class TransferFunction
    * @brief Clase que representa un LtiSystem como una función de transferencia.
    * 
    * Esta es solo una de las formas de definir un LtiSystem, forma parte de una jerarquía cuya raíz es una Planta.
    * 
    * @author Isaac Martínez Forte
   */

class TransferFunction : public LtiSystem
{
public:
  
  /**
    * @fn TransferFunction
    * @brief Constructor de la clase a partir del datos separados en variables.
    *  
    * @param nombre de la planta.
    * @param numerador de la función de transferencia.
    * @param denominador de la función de transferencia.
    * @param k ganancia asociada a la función de transferencia.
    * @param ret retardo asociado a la función de transferencia.
    */
    TransferFunction(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador, Parameter * k, Parameter* ret);
    
    virtual LtiSystem * create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret = NULL, QString exp_nume = 0, QString exp_deno = 0) = 0;
    
    
    /**
     * @fn ~TransferFunction
     * @brief Destructor de la clase.
     */

    ~TransferFunction();


    std::complex <qreal> evaluate (qreal omega);

    QVector <std::complex <qreal> > * evaluate (QVector <qreal> * omega);

    std::complex <qreal> evaluate (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega);

    virtual QString expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega) = 0;

    virtual QString expression(qreal w) = 0;

    virtual QString expression() = 0;

    virtual std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) = 0;
    
  /**
    * @fn numerator
    * @brief Función que devuelve el numerador de la función de transferencia.
    * 
    * @return vector con el numerador.
    */
  
    QVector <Parameter*> * numerator();

    void releaseOwnership ();
    
  /**
    * @fn denominator
    * @brief Función que devuelve el denominador de la función de transferencia.
    * 
    * @return vector con el denominador.
    */
  
    QVector <Parameter*> * denominator();
    
    QString numeratorString();
    QString denominatorString();

   /**
    * @fn gain
    * @brief Función que devuelve la ganancia de la función de transferencia.
    * 
    * @return real con la ganancia.
    */

    Parameter * gain();
    
    
   /**
    * @fn delay
    * @brief Función que devuelve el retardo de la función de transferencia.
    * 
    * @return real con el retardo.
    */
    
    Parameter * delay();
    
    
   /**
    * @fn type
    * @brief Función que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    virtual SystemType type() = 0;

    LtiSystem * clone ();

protected:
    Parameter * k;
    Parameter * ret;

    QVector <Parameter*> * numerador;
    QVector <Parameter*> * denominador;

    bool b = true;

};


#endif // QFTBX_TRANSFER_FUNCTION_H
