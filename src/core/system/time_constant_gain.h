#ifndef QFTBX_TIME_CONSTANT_GAIN_H
#define QFTBX_TIME_CONSTANT_GAIN_H

#include <QString>

#include "transfer_function.h"
#include "mpParser.h"


 /**
    * @class TimeConstantGain
    * @brief Clase que representa una planta del tipo TimeConstantGain, hereda de TransferFunction
    * 
    * Esta clase está dentro de una jerarquía que representa los distintos tipos de plantas que hay.
    * 
    * @author Isaac Martínez Forte
   */

class TimeConstantGain : public TransferFunction
{

public:
 
  /**
    * @fn TimeConstantGain
    * @brief Función que construye el objeto, tiene como parámetros los valores básicos de la planta.
    * 
    * @param nombre QString que indica el nombre de la planta.
    * @param numerador QVector de Variable que contiene el numerador completo de la planta.
    * @param denominador QVector de Variable que contiene el denominador cumpleto de la planta.
    * @param K Variable que contiene la ganancia de la planta.
    * @param ret Variable que contiene el retardo de la planta.
   */
  
    TimeConstantGain(QString nombre, QVector<Parameter *> *numerador, QVector<Parameter *> *denominador, Parameter *k, Parameter *ret);
    
    LtiSystem * create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret, QString exp_nume = 0, QString exp_deno = 0);
    
    /**
     * @fn ~TimeConstantGain
     * @brief Destructor de la clase.
     */
    
    ~TimeConstantGain();


   /**
    * @fn type
    * @brief Función que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    SystemType type();


    QString expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega);

    QString expression(qreal w);

    QString expression();


    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega);

};

#endif // QFTBX_TIME_CONSTANT_GAIN_H
