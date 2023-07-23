#ifndef SISTEMA_H
#define SISTEMA_H

#include <QString>
#include <complex>

#include "../EstructurasDatos/var.h"
#include "muparserx/mpParser.h"

  /**
    * @class Sistema 
    * @brief Clase que representa una sistema en el sistema, es la cabeza de una jerarquía donde están representados todos los tipos de sistema.
    * 
    * Esta clase es abstracta por lo tanto no puede ser instanciada, sirve para agrupar los distintos tipos de sistema en una jerarquía.
    * 
    * @author Isaac Martínez Forte
  
  */

class Sistema
{
public:
  
 /**
    * @fn Sistema 
    * @brief Constructor de la clase, solo tiene como parámetros el nombre del sistema a crear.
    * 
    * @param nombre cadena que contiene el nombre con que se quiere crear el sistema.
   */
  
    Sistema(QString nombre);

    virtual std::shared_ptr<Sistema> invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret = NULL, QString exp_nume = 0, QString exp_deno = 0) = 0;
    
    
    /**
     * @fn ~Sistema
     * @brief Destructor virtual de la clase.
     */

    virtual ~Sistema() {}
    
    
   /**
    * @fn setNombre 
    * @brief Función que sirve para cambiar el nombre de la sistema guardada.
    * 
    * @param nombre cadena con el nuevo nombre de la sistema.
    */

    void setNombre (QString nombre);
    
    
  /**
    * @fn getNombre
    * @brief Función que devuelve el nombre de la sistema.
    * 
    * @return cadena con el nombre de la sistema guardada.
    */
    
    QString getNombre();

  /**
    * @fn getPunto
    * @brief Función virtual pura virtual pura que resuelve la función de transferencia a partir del numerador, denominador, ganancia y retardo pasado por parámetros para una frecuencia de diseño concreta.
    * 
    * @param numerador utilizado para resolver la función de transferencia.
    * @param denominador utilizado para resolver la función de transferencia
    * @param w qreal que representa la frecuencia a utilizar.
    * @param k ganancia utilizada para resolver la función de transferencia.
    * @param ret retardo utilizado para resolver la función de transferencia.
    * 
    * @return complejo con el valor resuelto de la función de transferencia.
    * 
    * @see std/complex
    */

    virtual std::complex <qreal> getPunto (qreal omega) = 0;
    virtual QVector <std::complex <qreal> > * getPunto (QVector <qreal> * omega) = 0;

    virtual std::complex <qreal> getPunto (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega) = 0;

    virtual QString getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega) = 0;

    virtual QString getExpr(qreal w) = 0;

    virtual QString getExpr() = 0;

    virtual std::complex <qreal> getPuntoNume(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> getPuntoDeno(QVector <qreal> * deno, qreal omega) = 0;



   /**
    * @fn getDenominador
    * @brief Función virtual pura que retorna el denominador de la sistema.
    * 
    * @return denominador de la sistema.
    * 
    */
    
    virtual QVector <Var*> * getDenominador() = 0;
    
    
   /**
    * @fn getNumerador
    * @brief Función virtual pura que retorna el numerador de la sistema.
    * 
    * @return numerador de la sistema.
    * 
    */
    
    virtual QVector <Var*> * getNumerador() = 0;

    virtual QString getNumeradorString() = 0;
    virtual QString getDenominadorString() = 0;
    
    
   /**
    * @fn getK
    * @brief Función virtual pura que devuelve la ganancia de la función de transferencia.
    * 
    * @return real con la ganancia.
    */

    virtual Var * getK () = 0;
    
    
   /**
    * @fn getRet
    * @brief Función virtual pura que devuelve el retardo de la función de transferencia.
    * 
    * @return real con el retardo.
    */
    
    virtual Var * getRet() = 0;
    
    
   /**
    * @fn getClass
    * @brief Función virtual pura que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    enum tipo_planta {formato_libre, k_ganancia, k_no_ganancia, cof_polinomios};

    virtual tipo_planta getClass () = 0;

    virtual void noBorrar () = 0;

    virtual std::shared_ptr<Sistema> clone () = 0;
    
    
private:
    QString nombre;
    bool penalizacion;
};

#endif // SISTEMA_H
