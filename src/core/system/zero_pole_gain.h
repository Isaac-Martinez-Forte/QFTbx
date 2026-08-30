#ifndef QFTBX_ZERO_POLE_GAIN_H
#define QFTBX_ZERO_POLE_GAIN_H

#include "transfer_function.h"
#include "mpParser.h"

#include <QString>
#include <QDebug>



  /**
    * @class ZeroPoleGain
    * @brief Clase que representa una planta del tipo ZeroPoleGain, hereda de TransferFunction
    * 
    * Esta clase está dentro de una jerarquía que representa los distintos tipos de plantas que hay.
    * 
    * @author Isaac Martínez Forte
   */

class ZeroPoleGain : public TransferFunction
{

public:
  
  /**
    * @fn ZeroPoleGain
    * @brief Constructor de la clase.
    * 
    * @param nombre cadena que indica el nombre de la planta.
    * @param numerador vector de Variable que contiene el numerador de la planta.
    * @param denominador vector de Variable que contiene el denominador de la planta.
    * @param K Variable que contiene la ganancia de la planta.
    * @param ret Variable que contiene el retardo de la planta.
   */
  
    ZeroPoleGain(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador, Parameter * k, Parameter* ret);
    
    LtiSystem * create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret = NULL, QString exp_nume = 0, QString exp_deno = 0);
    
    /**
     * @fn ~ZeroPoleGain
     * @brief Destructor de la clase.
     */
    
    ~ZeroPoleGain();

    QString expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega);

    QString expression(qreal w);

    QString expression();

    std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega);
    
   /**
    * @fn type
    * @brief Función que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    SystemType type();

};

#endif // QFTBX_ZERO_POLE_GAIN_H
