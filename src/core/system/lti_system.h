#ifndef QFTBX_LTI_SYSTEM_H
#define QFTBX_LTI_SYSTEM_H

#include <QString>
#include <complex>

#include "src/core/system/parameter.h"
#include "mpParser.h"

namespace qftbx {

  /**
    * @class LtiSystem 
    * @brief Clase que representa una sistema en el sistema, es la cabeza de una jerarquía donde están representados todos los tipos de sistema.
    * 
    * Esta clase es abstracta por lo tanto no puede ser instanciada, sirve para agrupar los distintos tipos de sistema en una jerarquía.
    * 
    * @author Isaac Martínez Forte
  
  */

class LtiSystem
{
public:
  
 /**
    * @fn LtiSystem 
    * @brief Constructor de la clase, solo tiene como parámetros el nombre del sistema a crear.
    * 
    * @param nombre cadena que contiene el nombre con que se quiere crear el sistema.
   */
  
    LtiSystem(QString nombre);

    virtual LtiSystem * create (QString nombre, QVector <Parameter*> * numerador, QVector <Parameter*> * denominador,
                              Parameter * k, Parameter* ret = NULL, QString exp_nume = 0, QString exp_deno = 0) = 0;
    
    
    /**
     * @fn ~LtiSystem
     * @brief Destructor virtual de la clase.
     */

    virtual ~LtiSystem() {}
    
    
   /**
    * @fn setName 
    * @brief Función que sirve para cambiar el nombre de la sistema guardada.
    * 
    * @param nombre cadena con el nuevo nombre de la sistema.
    */

    void setName (QString nombre);
    
    
  /**
    * @fn name
    * @brief Función que devuelve el nombre de la sistema.
    * 
    * @return cadena con el nombre de la sistema guardada.
    */
    
    QString name();

  /**
    * @fn evaluate
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

    virtual std::complex <qreal> evaluate (qreal omega) = 0;
    virtual QVector <std::complex <qreal> > * evaluate (QVector <qreal> * omega) = 0;

    virtual std::complex <qreal> evaluate (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega) = 0;

    virtual QString expression (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega) = 0;

    virtual QString expression(qreal w) = 0;

    virtual QString expression() = 0;

    virtual std::complex <qreal> evaluateNumerator(QVector <qreal> * nume, qreal omega) = 0;

    virtual std::complex <qreal> evaluateDenominator(QVector <qreal> * deno, qreal omega) = 0;



   /**
    * @fn denominator
    * @brief Función virtual pura que retorna el denominador de la sistema.
    * 
    * @return denominador de la sistema.
    * 
    */
    
    virtual QVector <Parameter*> * denominator() = 0;
    
    
   /**
    * @fn numerator
    * @brief Función virtual pura que retorna el numerador de la sistema.
    * 
    * @return numerador de la sistema.
    * 
    */
    
    virtual QVector <Parameter*> * numerator() = 0;

    virtual QString numeratorString() = 0;
    virtual QString denominatorString() = 0;
    
    
   /**
    * @fn gain
    * @brief Función virtual pura que devuelve la ganancia de la función de transferencia.
    * 
    * @return real con la ganancia.
    */

    virtual Parameter * gain () = 0;
    
    
   /**
    * @fn delay
    * @brief Función virtual pura que devuelve el retardo de la función de transferencia.
    * 
    * @return real con el retardo.
    */
    
    virtual Parameter * delay() = 0;
    
    
   /**
    * @fn type
    * @brief Función virtual pura que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    enum class SystemType {FreeForm, ZeroPoleGain, TimeConstantGain, PolynomialForm};

    virtual SystemType type () = 0;

    virtual void releaseOwnership () = 0;

    virtual LtiSystem * clone () = 0;
    
    
private:
    QString nombre;
    bool penalizacion;
};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::LtiSystem;

#endif // QFTBX_LTI_SYSTEM_H
