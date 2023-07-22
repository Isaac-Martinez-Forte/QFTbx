#ifndef KNGANANCIA_H
#define KNGANANCIA_H

#include <QString>

#include "polosceros.h"
#include "muparserx/mpParser.h"


 /**
    * @class KNGanancia
    * @brief Clase que representa una planta del tipo KNGanancia, hereda de PolosCeros
    * 
    * Esta clase está dentro de una jerarquía que representa los distintos tipos de plantas que hay.
    * 
    * @author Isaac Martínez Forte
   */

class KNGanancia : public PolosCeros
{

public:
 
  /**
    * @fn KNGanancia
    * @brief Función que construye el objeto, tiene como parámetros los valores básicos de la planta.
    * 
    * @param nombre QString que indica el nombre de la planta.
    * @param numerador QVector de Variable que contiene el numerador completo de la planta.
    * @param denominador QVector de Variable que contiene el denominador cumpleto de la planta.
    * @param K Variable que contiene la ganancia de la planta.
    * @param ret Variable que contiene el retardo de la planta.
   */
  
    KNGanancia(QString nombre, QVector<Var *> *numerador, QVector<Var *> *denominador, Var *k, Var *ret);
    
    Sistema * invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret, QString exp_nume = 0, QString exp_deno = 0);
    
    /**
     * @fn ~KNGanancia
     * @brief Destructor de la clase.
     */
    
    ~KNGanancia();


   /**
    * @fn getClass
    * @brief Función que retorna la clase de la instancia.
    * 
    * @return cadena con la clase de la instancia.
    */

    tipo_planta getClass();


    QString getExpr (QVector <qreal> * numerador, QVector <qreal> * denominador,
                             qreal k, qreal ret, qreal omega);

    QString getExpr(qreal w);

    QString getExpr();


    std::complex <qreal> getPuntoNume(QVector <qreal> * nume, qreal omega);

    std::complex <qreal> getPuntoDeno(QVector <qreal> * deno, qreal omega);

};

#endif // KNGANANCIA_H
