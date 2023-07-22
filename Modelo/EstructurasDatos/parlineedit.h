#ifndef PARLABEL_H
#define PARLABEL_H

#include <QVector>
#include <QLineEdit>


  /**
    * @class ParLineEdit
    * @brief Clase que sirve para guardar tres QLineEdit de forma temporal para ser utilizados posteriormente.
    * 
    * @author Isaac Martínez Forte
   */

class ParLineEdit
{
public:
  
  /**
    * @fn ParLineEdit
    * @brief Constructor por defecto sin parámetros.
    * 
   */
  
    ParLineEdit();
    
  /**
    * @fn ParLineEdit
    * @brief Constructor de la clase con los tres parámetros básicos que guarda la clase.
    * 
    * @param x QLineEdit, es el primer objeto a guardar.
    * @param y QLineEdit, es el segundo objeto a guardar.
    * @param nominal QLineEdit, es el tercero objeto a guardar.
    * 
   */
    
    ParLineEdit(QLineEdit * x, QLineEdit * y, QLineEdit * nominal);
    
    
    /**
     * @fn ~ParLineEdit
     * @brief Destructor de la clase.
     */
    
    ~ParLineEdit();
    
   /**
    * @fn setX
    * @brief Función que guarda X en el sistema.
    * 
    * Si hubiera ya otro objeto guardardo anteriormente este sería borrado para que no haya fugas de memoria.
    * 
    * @param X a guardar en el sistema.
    * 
   */

    void setX (QLineEdit * label);
    
   /**
    * @fn getX
    * @brief Función que devuelve el objeto X guardado en el sistema.
    * 
    * @return Objeto X guardado en el sistema.
    * 
   */
    
    QLineEdit *getX();
    
   /**
    * @fn setY
    * @brief Función que guarda y en el sistema.
    * 
    * Si hubiera ya otro objeto guardardo anteriormente este sería borrado para que no haya fugas de memoria.
    * 
    * @param Y a guardar en el sistema.
    * 
   */
    
    void setY (QLineEdit * label);
    
   /**
    * @fn getY
    * @brief Función que devuelve el objeto y guardado en el sistema.
    * 
    * @return Objeto Y guardado en el sistema.
    * 
   */
    
    QLineEdit *getY();
    
   /**
    * @fn setNominal
    * @brief Función que guarda el nominal en el sistema.
    * 
    * Si hubiera ya otro objeto guardardo anteriormente este sería borrado para que no haya fugas de memoria.
    * 
    * @param nominal a guardar en el sistema.
    * 
   */
    
    void setNominal (QLineEdit *  nominal);
    
   /**
    * @fn getNominal
    * @brief Función que devuelve el objeto y guardado en el sistema.
    * 
    * @return Objeto nominal guardado en el sistema.
    * 
   */
    
    QLineEdit * getNominal();

private:
    QLineEdit *x;
    QLineEdit *y;
    QLineEdit * nominal;
};

#endif // PARLABEL_H
