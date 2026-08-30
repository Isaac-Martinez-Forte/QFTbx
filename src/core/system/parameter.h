#ifndef QFTBX_PARAMETER_H
#define QFTBX_PARAMETER_H

#include <QString>
#include <QPointF>
#include <QVector>

#include "mpParser.h"
#include "mpValue.h"
#include "mpVariable.h"

namespace qftbx {


  /**
    * @class Variable
    * @brief Clase que representa una Variable en el sistema.
    * 
    * Una Variable completa está compuesta de un nombre, un Rango y un nominal, aún que puede ser solo un valor nominal.
    * 
    * @author Isaac Martínez Forte
    */


class Parameter
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

    Parameter(QString nombre, QPointF rango, qreal nominal, QString exp);

    Parameter(QString nombre, QPointF rango, qreal nominal);

    Parameter(QPointF rango);

    Parameter (const Parameter &obj);

    Parameter();

    Parameter * clone ();

   /**
    * @fn cloneVector
    * @brief Copia profunda de un vector de variables (clona cada Parameter).
    *
    * @param origen vector a copiar; el llamante recibe la propiedad de la copia.
    */
    static QVector <Parameter*> * cloneVector(QVector <Parameter*> * origen);



  /**
    * @fn Variable
    * @brief Constructor de la clase para aquellas variables que solo tengan un valor nominal
    * 
    * @param valor de la variable que se guardará como nominal.
    */
    
    Parameter (qreal valor);
    Parameter (QString nombre, qreal valor);
    
  /**
    * @fn setName
    * @brief Función que guarda el nombre de la variable.
    * 
    * @param nombre cadena con el nombre de la variable.
    */
    
    
    void setName(QString nombre);
    
    
  /**
    * @fn setRange
    * @brief Función que guarda el rango de la variable.
    * 
    * @param rango del tipo Rango para guardar en el a variable.
    */
    
    
    void setRange (QPointF rango);
    
    
  /**
    * @fn setNominal
    * @brief Función que guarda el nominal de la variable
    * 
    * @param nominal qreal a guardar en la variable.
    */
    
    
    void setNominal(qreal nominal);
    
    
  /**
    * @fn isUncertain
    * @brief Función que retorna TRUE si la Variable es completa, FALSE en caso de que solo sea un nominal.
    * 
    * @return boolean que indica si la variables es completa o no.
    */
    
    
    bool isUncertain ();

    void setUncertain (bool a);
    
    
  /**
    * @fn name
    * @brief Función que retorna el nombre de la variable.
    * 
    * @return cadena con el nombre de la variable.
    */

    QString name();
    
    
  /**
    * @fn range
    * @brief Función que retorna el rango de la variable.
    * 
    * @return Rango con el rango de la variable.
    */
    
    QPointF range();
    QPointF rawRange();

    
  /**
    * @fn nominal
    * @brief Función que retorna el el nominal de la variable.
    * 
    * @return real con el nominal de la variable.
    */
    
    qreal nominal();
    qreal rawNominal();   
    QString expression();

private:
    QString nombre;
    QPointF rango;
    qreal m_nominal;
    bool variable;
    QString exp;
    bool e;

};

} // namespace qftbx

//Transitional: unqualified name for consumers not yet migrated
//to the qftbx namespace. Remove when the migration is complete.
using qftbx::Parameter;

#endif // QFTBX_PARAMETER_H
