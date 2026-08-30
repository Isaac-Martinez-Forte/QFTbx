#ifndef QFTBX_POLYNOMIAL_FORM_H
#define QFTBX_POLYNOMIAL_FORM_H

#include "transfer_function.h"

#include <QString>
#include "mpParser.h"

 /**
    * @class PolynomialForm
    * @brief Clase que reprepenta un coeficiente de polinomios, dicha clase es la parte principal de una Planta.
    * Esta es solo una de las formas de definir una Planta, forma parte de una jerarquía cuya raíz es una Planta.
    * 
    * @author Isaac Martínez Forte
   */


class PolynomialForm : public TransferFunction
{

public:
  
  
  /**
    * @fn PolynomialForm
    * @brief Constructor que crea la clase.
    * 
    * @param nombre de la planta.
    * @param numerador del coeficiente de polinomios.
    * @param denominador del coeficiente de polinomios.
    * @param k ganancia asociada al coeficiente de polinomios.
    * @param retardo asociado al coeficiente de polinomios.
   */
  
    PolynomialForm(QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador, Parameter * k, Parameter* ret);
    
    LtiSystem * create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret, QString exp_nume = 0, QString exp_deno = 0);

   /**
    * @fn ~PolynomialForm
    * @brief Destructor que crea la clase.
   */
    
    ~PolynomialForm();


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

#endif // QFTBX_POLYNOMIAL_FORM_H
