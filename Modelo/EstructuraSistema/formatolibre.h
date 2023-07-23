#ifndef FORMATOLIBRE_H
#define FORMATOLIBRE_H

#include <QVector>

#include "sistema.h"
#include "complex"
#include "muparserx/mpParser.h"

 /**
    * @class FormatoLibre
    * @brief Clase que reprepenta una Planta definida por datos experimentales.
    * 
    * Esta es solo una de las formas de definir una Planta, forma parte de una jerarquía cuya raíz es Planta.
    * @author Isaac Martínez Forte
   */

class FormatoLibre : public Sistema
{
public:
  
  
   /**
    * @fn FormatoLibre
    * @brief Constructor que crea el objeto.
    * 
    * @param nombre de la planta a guardar en el sistema.
    * @param fase QVector de qreal que define la fase de la planta.
    * @param magnitud QVector de qreal que define la magnitud de la planta.
    * @param frecuencias QVector de qreal que define las frecuencias de la planta. 
    */
 
    FormatoLibre(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret, QString exp_nume,
                 QString exp_deno);

    
    
   /**
    * @fn DatosExperimentales
    * @brief Constructor que crea el objeto a partir de otro objeto de la misma clase.
    * 
    * @param datos objeto de la misma clase a copiar.
    */
    
    FormatoLibre(FormatoLibre *datos);
    
    
   /**
    * @fn ~DatosExperimentales
    * @brief Destructor que crea el objeto.
    */
    
    ~FormatoLibre();


  /**
    * @fn getPunto
    * @brief Función que retorna un número complejo que resuelve la planta junto w que es la frecuencia de diseño..
    * 
    * @param w real que representa la frecuencia utilizada para resolver la Planta.
    * 
    * @see std/complex
    * 
   */
  
    std::complex<qreal> getPunto(qreal w);

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

    QVector <std::complex <qreal> > * getPunto (QVector <qreal> * omega);

    std::complex <qreal> getPunto (QVector <qreal> * numerador, QVector <qreal> * denominador,
                                           qreal k, qreal ret, qreal omega);

    QVector <Var*> * getDenominador();

    QVector <Var*> * getNumerador();

    Var * getK ();

    Var * getRet();


    std::shared_ptr<Sistema> invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret, QString exp_nume = 0, QString exp_deno = 0);

    QString getNumeradorString();
    QString getDenominadorString();

    void noBorrar ();

    std::shared_ptr<Sistema> clone();

private:
    Var * k;
    Var * ret;

    QVector <Var*> * numerador;
    QVector <Var*> * denominador;

    QString exp_nume;
    QString exp_deno;

    bool b = true;
};

#endif // DATOSEXPERIMENTALES_H
