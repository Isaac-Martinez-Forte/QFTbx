#ifndef FUNCIONTRANSFERENCIA_H
#define FUNCIONTRANSFERENCIA_H

#include "sistema.h"
#include <QVector>
#include "../EstructurasDatos/var.h"
#include "muparserx/mpParser.h"

 /**
    * @class FuncionTransferencia
    * @brief Clase que representa un Sistema como una función de transferencia.
    * 
    * Esta es solo una de las formas de definir un Sistema, forma parte de una jerarquía cuya raíz es una Planta.
    * 
    * @author Isaac Martínez Forte
   */

class FuncionTransferencia : public Sistema
{
public:
  
  /**
    * @fn FuncionTransferencia
    * @brief Constructor de la clase a partir del datos separados en variables.
    *  
    * @param nombre de la planta.
    * @param numerador de la función de transferencia.
    * @param denominador de la función de transferencia.
    * @param k ganancia asociada a la función de transferencia.
    * @param ret retardo asociado a la función de transferencia.
    */
    FuncionTransferencia(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret);
    
  /**
    * @fn FuncionTransferencia
    * @brief Constructor de la clase a partir de una FuncionTransferencia ya definida.
    *  
    * @param datos función de transferencia a partir de la cual queremos construir el objeto.
    */ 
 
    FuncionTransferencia (FuncionTransferencia *datos);


    virtual std::shared_ptr<Sistema> invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret = NULL, QString exp_nume = 0, QString exp_deno = 0) = 0;
    
    
    /**
     * @fn ~FuncionTransferencia
     * @brief Destructor de la clase.
     */

    ~FuncionTransferencia();


    std::complex <qreal> getPunto (qreal omega);

    QVector <std::complex <qreal> > * getPunto (QVector <qreal> * omega);

    std::complex <qreal> getPunto (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega);

    virtual QString getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega) = 0;

    virtual QString getExpr(qreal w) = 0;

    virtual QString getExpr() = 0;

    virtual std::complex <qreal> getPuntoNume(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> getPuntoDeno(QVector <qreal> * deno, qreal omega) = 0;
    
  /**
    * @fn getNumerador
    * @brief Función que devuelve el numerador de la función de transferencia.
    * 
    * @return vector con el numerador.
    */
  
    QVector <Var*> * getNumerador();

    void noBorrar ();
    
  /**
    * @fn getDenominador
    * @brief Función que devuelve el denominador de la función de transferencia.
    * 
    * @return vector con el denominador.
    */
  
    QVector <Var*> * getDenominador();
    
    QString getNumeradorString();
    QString getDenominadorString();

   /**
    * @fn getK
    * @brief Función que devuelve la ganancia de la función de transferencia.
    * 
    * @return real con la ganancia.
    */

    Var * getK();
    
    
   /**
    * @fn getRet
    * @brief Función que devuelve el retardo de la función de transferencia.
    * 
    * @return real con el retardo.
    */
    
    Var * getRet();
    
    
   /**
    * @fn getClass
    * @brief Función que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    virtual tipo_planta getClass() = 0;

    std::shared_ptr<Sistema> clone();

protected:
    Var * k;
    Var * ret;

    QVector <Var*> * numerador;
    QVector <Var*> * denominador;

    bool b = true;

};


#endif // FUNCIONTRANSFERENCIA_H
