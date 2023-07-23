#ifndef CPOLINOMIOS_H
#define CPOLINOMIOS_H

#include "funciontransferencia.h"

#include <QString>
#include "muparserx/mpParser.h"

 /**
    * @class CPolinomios
    * @brief Clase que reprepenta un coeficiente de polinomios, dicha clase es la parte principal de una Planta.
    * Esta es solo una de las formas de definir una Planta, forma parte de una jerarquía cuya raíz es una Planta.
    * 
    * @author Isaac Martínez Forte
   */


class CPolinomios : public FuncionTransferencia
{

public:
  
  
  /**
    * @fn CPolinomios
    * @brief Constructor que crea la clase.
    * 
    * @param nombre de la planta.
    * @param numerador del coeficiente de polinomios.
    * @param denominador del coeficiente de polinomios.
    * @param k ganancia asociada al coeficiente de polinomios.
    * @param retardo asociado al coeficiente de polinomios.
   */
  
    CPolinomios(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret);
    
    std::shared_ptr<Sistema> invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret, QString exp_nume = 0, QString exp_deno = 0);

   /**
    * @fn ~CPolinomios
    * @brief Destructor que crea la clase.
   */
    
    ~CPolinomios();


    QString getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega);

    QString getExpr(qreal w);

    QString getExpr();

    std::complex <qreal> getPuntoNume(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> getPuntoDeno(QVector <qreal> * deno, qreal omega);
    
   /**
    * @fn getClass
    * @brief Función que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */
    
    tipo_planta getClass();
};

#endif // CPOLINOMIOS_H
