#ifndef KGANANCIA_H
#define KGANANCIA_H

#include "polosceros.h"
#include "muparserx/mpParser.h"

#include <QString>
#include <QDebug>



  /**
    * @class KGanancia
    * @brief Clase que representa una planta del tipo KGanancia, hereda de PolosCeros
    * 
    * Esta clase está dentro de una jerarquía que representa los distintos tipos de plantas que hay.
    * 
    * @author Isaac Martínez Forte
   */

class KGanancia : public PolosCeros
{

public:
  
  /**
    * @fn KGanancia
    * @brief Constructor de la clase.
    * 
    * @param nombre cadena que indica el nombre de la planta.
    * @param numerador vector de Variable que contiene el numerador de la planta.
    * @param denominador vector de Variable que contiene el denominador de la planta.
    * @param K Variable que contiene la ganancia de la planta.
    * @param ret Variable que contiene el retardo de la planta.
   */
  
    KGanancia(QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador, Var * k, Var* ret);
    
    Sistema * invoke (QString nombre, QVector <Var*> * numerador, QVector <Var*> * denominador,
                              Var * k, Var* ret = NULL, QString exp_nume = 0, QString exp_deno = 0);
    
    /**
     * @fn ~KGanancia
     * @brief Destructor de la clase.
     */
    
    ~KGanancia();

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

#endif // KGANANCIA_H
