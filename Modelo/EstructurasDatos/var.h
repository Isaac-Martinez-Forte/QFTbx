#ifndef VAR_H
#define VAR_H

#include <QString>
#include <QPointF>

#include "muparserx/mpParser.h"
#include "muparserx/mpValue.h"
#include "muparserx/mpVariable.h"


  /**
    * @class Variable
    * @brief Clase que representa una Variable en el sistema.
    * 
    * Una Variable completa está compuesta de un nombre, un Rango y un nominal, aún que puede ser solo un valor nominal.
    * 
    * @author Isaac Martínez Forte
    */


class Var
{
public:
  
  
  /**
    * @fn Variable
    * @brief Constructor completo de la clase.
    * 
    * @param nombre cadena con el nombre de la variable.
    * @param rango de la variable.
    * @param nominal de la variable.
    */

    Var(QString nombre, QPointF rango, qreal nominal, QString exp);

    Var(QString nombre, QPointF rango, qreal nominal);

    Var(QPointF rango);

    Var (const Var &obj);

    Var();

    Var * clone ();
    
    
  /**
    * @fn Variable
    * @brief Constructor de la clase para aquellas variables que solo tengan un valor nominal
    * 
    * @param valor de la variable que se guardará como nominal.
    */
    
    Var (qreal valor);
    Var (QString nombre, qreal valor);
    
  /**
    * @fn setNombre
    * @brief Función que guarda el nombre de la variable.
    * 
    * @param nombre cadena con el nombre de la variable.
    */
    
    
    void setNombre(QString nombre);
    
    
  /**
    * @fn setRango
    * @brief Función que guarda el rango de la variable.
    * 
    * @param rango del tipo Rango para guardar en el a variable.
    */
    
    
    void setRango (QPointF rango);
    
    
  /**
    * @fn setNominal
    * @brief Función que guarda el nominal de la variable
    * 
    * @param nominal qreal a guardar en la variable.
    */
    
    
    void setNominal(qreal nominal);
    
    
  /**
    * @fn isVariable
    * @brief Función que retorna TRUE si la Variable es completa, FALSE en caso de que solo sea un nominal.
    * 
    * @return boolean que indica si la variables es completa o no.
    */
    
    
    bool isVariable ();

    void setVariable (bool a);
    
    
  /**
    * @fn getNombre
    * @brief Función que retorna el nombre de la variable.
    * 
    * @return cadena con el nombre de la variable.
    */

    QString getNombre();
    
    
  /**
    * @fn getRango
    * @brief Función que retorna el rango de la variable.
    * 
    * @return Rango con el rango de la variable.
    */
    
    QPointF getRango();
    QPointF getR();

    
  /**
    * @fn getNominal
    * @brief Función que retorna el el nominal de la variable.
    * 
    * @return real con el nominal de la variable.
    */
    
    qreal getNominal();
    qreal getN();   
    QString getExp();

private:
    QString nombre;
    QPointF rango;
    qreal nominal;
    bool variable;
    QString exp;
    bool e;

};

#endif // VAR_H
